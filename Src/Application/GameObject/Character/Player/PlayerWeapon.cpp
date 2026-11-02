// PlayerWeapon.cpp
// 剣（持たせる位置・刃の形・剣の軌跡・投げた剣の補正）
// ※Player クラスの関数を、役割ごとにファイルを分けて書いている（クラスの宣言は Player.h）

// Player
#include "Player.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"
#include "../UI/Gage/ATB/ATBGage.h"
#include "../UI/Gage/ATB/ATBCommandMenu.h"

// Enemy
#include "../Enemy/Enemy.h"

// Camera
#include "../../Camera/TPSCamera/TPSCamera.h"

// Effect
#include "../../Effect/EffectPlayer.h"

// Utility
#include "../../../Utility/HitStop.h"
#include "../../../Utility/SoundPlayer.h"
#include "../../../Utility/GameTime.h"

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

#include "PlayerConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerConst;

// GUIで調整した回転(度)・座標から、ノードに追従する行列を作成
Math::Matrix Player::CreateAttachMatrix(const Math::Vector3& rotDeg, const Math::Vector3& pos,
	const Math::Matrix& nodeMat, const Math::Matrix& ownerMat)
{
	Math::Matrix rot =
		Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(rotDeg.x))
		* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(rotDeg.y))
		* Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(rotDeg.z));
	Math::Matrix trans = Math::Matrix::CreateTranslation(pos);

	return rot * trans * nodeMat * ownerMat;
}

// 剣のローカルY軸（刃の方向）上の点をワールド座標に変換
Math::Vector3 Player::GetSwordPoint(const Math::Matrix& swordMat, float localY)
{
	return Math::Vector3::Transform(Math::Vector3(0.0f, localY, 0.0f), swordMat);
}

Player::BladeShape Player::CalcBladeShape(const KdModelData& modelData, const std::vector<std::string>& meshNodeNames)
{
	BladeShape shape;

	// 剣のメッシュのノードを探す
	const auto& nodes = modelData.GetOriginalNodes();
	const std::vector<Math::Vector3>* pPositions = nullptr;
	for (const auto& name : meshNodeNames)
	{
		for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
		{
			if (nodes[i].m_name != name || !nodes[i].m_spMesh) continue;

			shape.nodeIndex = i;
			pPositions = &nodes[i].m_spMesh->GetVertexPositions();
			break;
		}
		if (pPositions) break;
	}
	if (!pPositions || pPositions->empty())
	{
		shape.nodeIndex = -1;
		return shape;
	}
	const auto& positions = *pPositions;

	// ① 頂点の中心
	Math::Vector3 center = Math::Vector3::Zero;
	for (const auto& pos : positions) { center += pos; }
	center /= static_cast<float>(positions.size());

	// ② 頂点が一番長く広がっている向き（＝刃の中心線）を求める（主成分分析）
	//    頂点の散らばり具合を表す行列を作り、その行列を何度も掛けると、一番散らばっている向きに近づく
	float cov[3][3] = {};
	for (const auto& pos : positions)
	{
		float d[3] = { pos.x - center.x, pos.y - center.y, pos.z - center.z };
		for (int r = 0; r < 3; ++r)
		{
			for (int c = 0; c < 3; ++c) { cov[r][c] += d[r] * d[c]; }
		}
	}
	Math::Vector3 axis = { 1.0f, 1.0f, 1.0f };
	axis.Normalize();
	for (int i = 0; i < 32; ++i)
	{
		Math::Vector3 next =
		{
			cov[0][0] * axis.x + cov[0][1] * axis.y + cov[0][2] * axis.z,
			cov[1][0] * axis.x + cov[1][1] * axis.y + cov[1][2] * axis.z,
			cov[2][0] * axis.x + cov[2][1] * axis.y + cov[2][2] * axis.z,
		};
		if (next.LengthSquared() <= 0.0f) break;
		next.Normalize();
		axis = next;
	}

	// ③ 中心線の上で、頂点が一番遠くまである位置（剣の両端）
	float minT = std::numeric_limits<float>::max();
	float maxT = -std::numeric_limits<float>::max();
	for (const auto& pos : positions)
	{
		float t = (pos - center).Dot(axis);
		minT = std::min(minT, t);
		maxT = std::max(maxT, t);
	}

	// ④ ノードの原点が手の位置なので、中心線の上で手に一番近い点を求め、
	//    手から遠い方の端を刃先とする
	float handT = (Math::Vector3::Zero - center).Dot(axis);
	shape.localHand = center + axis * handT;
	if (maxT - handT >= handT - minT)
	{
		shape.localDir = axis;
		shape.tipLength = maxT - handT;
	}
	else
	{
		shape.localDir = -axis;
		shape.tipLength = handT - minT;
	}

	return shape;
}

Math::Matrix Player::CalcBladeMatrix(const BladeShape& shape) const
{
	if (!m_model) return Math::Matrix::Identity;
	return CalcBladeMatrix(shape, *m_model, m_mWorld);
}

std::shared_ptr<KdTexture> Player::LoadOperatorTrailTexture()
{
	return LoadSwordTrailTexture(kOperatorTrailTexPath, kOperatorTrailColor);
}

Math::Matrix Player::CalcBladeMatrix(const BladeShape& shape, const KdModelWork& model, const Math::Matrix& mWorld)
{
	if (shape.nodeIndex < 0 || shape.nodeIndex >= static_cast<int>(model.GetNodes().size()))
	{
		return Math::Matrix::Identity;
	}

	// 剣のメッシュが今いる位置（モデルの中の位置 × キャラの位置）
	Math::Matrix nodeMat = model.GetNodes()[shape.nodeIndex].m_worldTransform * mWorld;

	Math::Vector3 hand = Math::Vector3::Transform(shape.localHand, nodeMat);
	Math::Vector3 up = Math::Vector3::TransformNormal(shape.localDir, nodeMat);
	up.Normalize();

	// X軸はノードのX軸を、刃の向きと直角になるように直したもの
	Math::Vector3 right = nodeMat.Right();
	right -= up * right.Dot(up);
	if (right.LengthSquared() < 1e-6f) { right = nodeMat.Backward().Cross(up); }
	right.Normalize();
	Math::Vector3 forward = right.Cross(up);

	Math::Matrix mat(right, up, forward);
	mat.Translation(hand);
	return mat;
}

// 剣の軌跡ポリゴンに渡す行列を作成
// KdTrailPolygonは行列のX軸方向に帯を作るので、X軸を刃の向き・長さに合わせる
Math::Matrix Player::CreateTrailMatrix(const Math::Vector3& base, const Math::Vector3& tip)
{
	// 帯の幅は「X軸の長さ × 0.5」になるので、刃の長さの2倍を設定する
	Math::Matrix trailMat = Math::Matrix::Identity;
	trailMat.Right((tip - base) * (kSwordTrailWidthRate * 2.0f));
	trailMat.Translation((base + tip) * 0.5f);
	return trailMat;
}

std::shared_ptr<KdTexture> Player::LoadSwordTrailTexture(const std::string& path, const Math::Vector3& fallbackColor)
{
	// 画像があればそれを使い、無ければ同じ見た目のものをプログラムで作る
	auto tex = std::make_shared<KdTexture>();
	if (tex->Load(path)) return tex;

	OutputDebugStringA(("【軌跡の画像が無いので、プログラムで作ります】" + path + "\n").c_str());
	return CreateSwordTrailTexture(fallbackColor);
}

void Player::ApplySwordTrailTexture()
{
	// 今のモードの画像に切り替える
	const std::shared_ptr<KdTexture>& tex = IsPunisherMode() ? m_punisherTrailTex : m_operatorTrailTex;
	if (!tex) return;

	for (SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		if (pTrail->polygon) pTrail->polygon->SetMaterial(tex);
	}
}

std::shared_ptr<KdTexture> Player::CreateSwordTrailTexture(const Math::Vector3& trailColor)
{
	// 0～1の範囲で滑らかに補間する
	auto smoothStep = [](float edge0, float edge1, float x)
	{
		float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	};

	std::vector<UINT> pixels(kSwordTrailTexWidth * kSwordTrailTexHeight);

	for (int y = 0; y < kSwordTrailTexHeight; ++y)
	{
		// 縦（V）：0が一番新しい位置、1が一番古い位置
		float v = (y + 0.5f) / kSwordTrailTexHeight;
		float fadeOld = std::pow(1.0f - v, 1.5f);			// 古いほど暗く
		float fadeNew = smoothStep(0.0f, 0.03f, v);		// 剣のすぐ後ろは少しだけぼかす
		float fadeV = fadeOld * fadeNew;

		for (int x = 0; x < kSwordTrailTexWidth; ++x)
		{
			// 横（U）：0が刃の先端側、1が刃の根元側
			float u = (x + 0.5f) / kSwordTrailTexWidth;

			// 先端ほど明るく、両端はふちをぼかす
			float bright = kSwordTrailBaseBrightness + (1.0f - kSwordTrailBaseBrightness) * std::pow(1.0f - u, 1.5f);
			float edge = smoothStep(0.0f, 0.06f, u) * smoothStep(1.0f, 0.8f, u);
			float intensity = bright * edge * fadeV;

			// 先端付近の新しい部分は白っぽい芯にする
			float core = std::pow(1.0f - u, 6.0f) * fadeOld;
			Math::Vector3 color = Math::Vector3::Lerp(trailColor, kSwordTrailCoreColor, core) * intensity;

			// 加算合成で描くので、透明度ではなく色の明るさで薄さを表現する
			// （UnLitシェーダーのアルファテストで切られないよう、アルファは常に1）
			UINT r = static_cast<UINT>(std::clamp(color.x, 0.0f, 1.0f) * 255.0f);
			UINT g = static_cast<UINT>(std::clamp(color.y, 0.0f, 1.0f) * 255.0f);
			UINT b = static_cast<UINT>(std::clamp(color.z, 0.0f, 1.0f) * 255.0f);
			pixels[y * kSwordTrailTexWidth + x] = (255u << 24) | (b << 16) | (g << 8) | r;
		}
	}

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = pixels.data();
	initData.SysMemPitch = kSwordTrailTexWidth * sizeof(UINT);

	auto tex = std::make_shared<KdTexture>();
	if (!tex->Create(kSwordTrailTexWidth, kSwordTrailTexHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &initData))
	{
		OutputDebugStringA("【剣の軌跡テクスチャの作成に失敗】\n");
		return nullptr;
	}
	return tex;
}

int Player::FindNodeIndex(const std::string& name) const
{
	if (!m_model || !m_model->GetData()) return -1;

	const auto& nodes = m_model->GetData()->GetOriginalNodes();
	for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
	{
		if (nodes[i].m_name == name) return i;
	}
	return -1;
}

float Player::CalcThrowCorrectionAngle(const std::string& animName, int weaponIndex) const
{
	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !m_model || !m_model->GetData()) return 0.0f;
	if (weaponIndex < 0 || m_pelvisNode < 0) return 0.0f;

	// 計算用にモデルを複製し、1フレームずつ再生して、剣が腰から一番離れたフレームを探す
	KdModelWork work(m_model->GetData());
	KdAnimator animator;
	animator.SetAnimation(it->second, false);

	Math::Vector3 farthest = Math::Vector3::Zero;
	float maxDistSq = 0.0f;
	for (int i = 0; i < 1000 && !animator.IsAnimationEnd(); ++i)
	{
		animator.AdvanceTime(work.WorkNodes(), 1.0f);
		work.CalcNodeMatrices();

		Math::Vector3 diff = work.GetNodes()[weaponIndex].m_worldTransform.Translation()
			- work.GetNodes()[m_pelvisNode].m_worldTransform.Translation();
		diff.y = 0.0f;
		if (diff.LengthSquared() > maxDistSq)
		{
			maxDistSq = diff.LengthSquared();
			farthest = diff;
		}
	}
	if (maxDistSq < 0.0001f) return 0.0f;

	// 剣が飛んだ向きを、モデルの前（-Z。描画で180度回しているため）に向ける回転角度
	Math::Vector3 dir = farthest;
	dir.Normalize();
	const Math::Vector3 forward = { 0.0f, 0.0f, -1.0f };
	float angle = std::atan2(dir.Cross(forward).y, dir.Dot(forward));

	// 回転の向き（符号）を確かめ、逆なら反対に回す
	Math::Vector3 rotated = Math::Vector3::TransformNormal(dir, Math::Matrix::CreateRotationY(angle));
	if (rotated.Dot(forward) < 0.99f) angle = -angle;

	return DirectX::XMConvertToDegrees(angle);
}

void Player::ApplyThrowCorrection()
{
	if (!m_model || m_currentAnimName != kThrowAnimName) return;

	CorrectThrownWeapon(m_weaponNodeR, m_handNodeR, m_throwAngleR + s_throwAngleOffset);
	CorrectThrownWeapon(m_weaponNodeL, m_handNodeL, m_throwAngleL + s_throwAngleOffset);

	// 剣の骨の位置を変えたので、子の骨（剣のモデル）の位置を計算し直す
	m_model->CalcNodeMatrices();
}

void Player::CorrectThrownWeapon(int weaponIndex, int handIndex, float angleDeg)
{
	if (weaponIndex < 0 || handIndex < 0 || m_pelvisNode < 0) return;

	auto& nodes = m_model->WorkNodes();
	const Math::Matrix& weaponWorld = nodes[weaponIndex].m_worldTransform;

	// 手から離れるほど強く補正する（手に持っている間は補正しない）
	float dist = (weaponWorld.Translation() - nodes[handIndex].m_worldTransform.Translation()).Length();
	float t = std::clamp((dist - kThrowCorrectStartDist) / (kThrowCorrectFullDist - kThrowCorrectStartDist), 0.0f, 1.0f);
	float weight = t * t * (3.0f - 2.0f * t);
	if (weight <= 0.0f) return;

	// 腰を中心に、剣を水平に回して前へ向ける
	Math::Vector3 pivot = nodes[m_pelvisNode].m_worldTransform.Translation();
	pivot.y = 0.0f;
	Math::Matrix correction = Math::Matrix::CreateTranslation(-pivot)
		* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(angleDeg * weight))
		* Math::Matrix::CreateTranslation(pivot);
	Math::Matrix newWorld = weaponWorld * correction;

	// 親（手）から見た行列に直して書き戻す
	int parent = m_model->GetData()->GetOriginalNodes()[weaponIndex].m_parent;
	Math::Matrix parentWorld = (parent >= 0) ? nodes[parent].m_worldTransform : Math::Matrix::Identity;
	nodes[weaponIndex].m_localTransform = newWorld * parentWorld.Invert();
}

void Player::UpdateWeaponMatrix()
{
	m_swordWorldR = Math::Matrix::Identity;
	m_swordWorldL = Math::Matrix::Identity;

	if (!m_model) return;

	// 攻撃判定・軌跡を剣のメッシュに合わせるか（モデルに剣が含まれている時だけ）
	bool useBladeShape = s_bladeShapeHitbox && m_bladeShapeR.nodeIndex >= 0;
	if (useBladeShape)
	{
		m_swordBladeBaseY = std::min(kBladeBaseOffset, m_bladeShapeR.tipLength * 0.5f);
		m_swordBladeTipY = m_bladeShapeR.tipLength;
	}
	else
	{
		m_swordBladeBaseY = kSwordBladeBaseY;
		m_swordBladeTipY = kSwordBladeTipY;
	}

	// 右
	m_swordWorldR = useBladeShape
		? CalcBladeMatrix(m_bladeShapeR)	// モデルに含まれる剣：メッシュの位置そのもの
		: CalcSwordMatrix(kSwordNodeNamesR, s_weaponRotR, s_weaponPosR);

	// 左
	m_swordWorldL = (useBladeShape && m_bladeShapeL.nodeIndex >= 0)
		? CalcBladeMatrix(m_bladeShapeL)
		: CalcSwordMatrix(kSwordNodeNamesL, s_weaponRotL, s_weaponPosL);
}

const KdModelWork::Node* Player::FindFirstNode(const std::vector<std::string>& nodeNames) const
{
	// モデルが読み込めていない場合は探さない
	if (!m_model || !m_model->IsEnable()) return nullptr;

	for (const auto& name : nodeNames)
	{
		if (const KdModelWork::Node* pNode = m_model->FindNode(name))
		{
			return pNode;
		}
	}
	return nullptr;
}

Math::Matrix Player::CalcSwordMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos) const
{
	const KdModelWork::Node* pNode = FindFirstNode(nodeNames);
	if (!pNode) return Math::Matrix::Identity;

	return CreateAttachMatrix(rot, pos, pNode->m_worldTransform, m_mWorld);
}

void Player::AddSwordTrailPoints()
{
	// 新しい振りの始まりなら、前の振りの軌跡とつながらないように消しておく
	if (!m_isTrailActive)
	{
		ClearSwordTrail(m_swordTrailR);
		ClearSwordTrail(m_swordTrailL);
	}

	AddSwordTrailSample(m_swordTrailR, m_swordWorldR);
	AddSwordTrailSample(m_swordTrailL, m_swordWorldL);

	m_isTrailActive = true;
	m_isTrailAddedThisFrame = true;
}

void Player::UpdateSwordTrails()
{
	if (m_isTrailAddedThisFrame) return;

	// 攻撃していないフレームは、古いポイントから消して軌跡を徐々に短くする
	m_isTrailActive = false;
	ShrinkSwordTrail(m_swordTrailR);
	ShrinkSwordTrail(m_swordTrailL);
}

void Player::AddSwordTrailSample(SwordTrail& trail, const Math::Matrix& swordMat)
{
	if (!trail.polygon) return;

	Math::Vector3 base = GetSwordPoint(swordMat, m_swordBladeBaseY);
	Math::Vector3 tip = GetSwordPoint(swordMat, m_swordBladeTipY);

	auto& baseHist = trail.baseHistory;
	auto& tipHist = trail.tipHistory;

	if (baseHist.empty())
	{
		// 最初のポイントはそのまま追加
		trail.polygon->AddPoint(CreateTrailMatrix(base, tip));
	}
	else
	{
		// 前フレームと今フレームの間を Catmull-Rom 曲線で補間して、カクつきのない弧にする
		// p0：前々フレーム（無ければ前フレーム）、p1：前フレーム、p2：今フレーム、p3：この先の予測位置
		Math::Vector3 base0 = (baseHist.size() >= 2) ? baseHist[1] : baseHist[0];
		Math::Vector3 tip0 = (tipHist.size() >= 2) ? tipHist[1] : tipHist[0];
		Math::Vector3 base1 = baseHist[0];
		Math::Vector3 tip1 = tipHist[0];
		Math::Vector3 base3 = base + (base - base1);
		Math::Vector3 tip3 = tip + (tip - tip1);

		for (int i = 1; i <= kSwordTrailSubdivision; ++i)
		{
			float t = static_cast<float>(i) / kSwordTrailSubdivision;
			Math::Vector3 b = Math::Vector3::CatmullRom(base0, base1, base, base3, t);
			Math::Vector3 e = Math::Vector3::CatmullRom(tip0, tip1, tip, tip3, t);
			trail.polygon->AddPoint(CreateTrailMatrix(b, e));
		}
	}

	// 補間に使う直近2フレーム分だけ覚えておく
	baseHist.push_front(base);
	tipHist.push_front(tip);
	while (baseHist.size() > 2) baseHist.pop_back();
	while (tipHist.size() > 2) tipHist.pop_back();
}

void Player::ClearSwordTrail(SwordTrail& trail)
{
	if (trail.polygon) trail.polygon->ClearPoints();
	trail.baseHistory.clear();
	trail.tipHistory.clear();
}

void Player::ShrinkSwordTrail(SwordTrail& trail)
{
	// 補間で1フレームあたり複数ポイント追加しているので、同じ数だけ消す
	for (int i = 0; i < kSwordTrailSubdivision; ++i)
	{
		if (!trail.polygon || trail.polygon->GetNumPoints() <= 0) break;
		trail.polygon->DelPointBack();
	}
}
