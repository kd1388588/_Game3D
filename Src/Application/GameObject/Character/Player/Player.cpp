// Player.cpp

// Player
#include "Player.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"
#include "../UI/Gage/ATB/ATBGage.h"

// Enemy
#include "../Enemy/Enemy.h"

// Camera
#include "../../Camera/TPSCamera/TPSCamera.h"

// Effect
#include "../../Effect/EffectPlayer.h"

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

using InputHelper::IsKeyDown;

Math::Vector3 Player::s_weaponRotR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponRotL = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosL = { 0.0f, 0.0f, 0.0f };

int Player::s_currentAttackType = 1;

bool Player::s_useRootMotion = true;
bool Player::s_bladeShapeHitbox = true;
float Player::s_throwAngleOffset = 0.0f;
float Player::s_runSpeedRate = 1.5f;

namespace
{
	// 移動
	constexpr float kMoveSpeed				= 0.15f;
	constexpr float kAwakeningMoveSpeed		= 0.30f;	// 覚醒中は2倍のスピード
	constexpr float kMaxTurnAngle			= 10.0f;	// 1フレームの最大旋回角度（度）
	constexpr float kMinTurnAngle			= 0.1f;		// これ未満の角度差は旋回しない

	// ステータス
	constexpr int	kMaxHp					= 100;
	constexpr int	kDamageInvincibleFrame	= 30;		// 被弾後の無敵時間
	constexpr int	kHeavyDamage			= 20;		// これ以上のダメージで仰け反る
	constexpr int	kAttackDamage			= 10;
	constexpr int	kCriticalAttackDamage	= 20;
	constexpr int	kAttackTypeNum			= 5;		// 攻撃パターンの数

	// バトルモード（FF7リメイクのクラウド風）
	// モードごとに使える攻撃パターン（Cキーでこの中を切り替える）
	const std::vector<int> kOperatorAttackTypes	= { 1, 2, 3 };	// オペレーター：手数の多い攻撃
	const std::vector<int> kPunisherAttackTypes	= { 4, 5 };		// パニッシャー：重い攻撃
	constexpr float kPunisherMoveRate		= 0.6f;		// パニッシャーの移動の速さ（オペレーターを1.0とした倍率）
	constexpr float kPunisherDamageRate		= 1.5f;		// パニッシャーのダメージの倍率

	// ATBゲージのたまり方
	constexpr float kAtbPassiveGain			= 0.05f;	// オペレーターの時、1フレームにたまる量（約33秒で1本）
	constexpr float kAtbHitGain				= 8.0f;		// 攻撃を当てた時にたまる量
	constexpr float kPunisherAtbHitRate		= 2.0f;		// パニッシャーの時、当てた時にたまる量の倍率
	constexpr float kAtbCriticalRate		= 1.5f;		// クリティカルの時、当てた時にたまる量の倍率

	// 覚醒
	constexpr int	kAwakeningFrame			= 60 * 15;	// 15秒間持続
	constexpr float	kAwakeningHealRate		= 0.3f;		// HP3割回復

	// 接地判定に使う重力の上限（落下し始めの1フレーム分）
	constexpr float kOnGroundGravityMax		= 0.011f;

	// 剣の刃の位置（剣のローカルY軸上。攻撃判定の箱と同じ範囲）
	// ※別ファイルの剣を持たせる場合に使う。モデルに剣が含まれている場合はメッシュから自動で求める
	constexpr float kSwordBladeBaseY		= 0.05f;	// 刃の根元
	constexpr float kSwordBladeTipY			= 1.15f;	// 刃の先端
	constexpr float kSwordOBBHalfWidth		= 0.1f;		// 攻撃判定の箱の太さ（半分）

	// モデルに含まれる剣のメッシュ（上から順に探して、最初に見つかったものを使う）
	const std::vector<std::string> kBladeMeshNodeNamesR	= { "SM_Blade_R" };
	const std::vector<std::string> kBladeMeshNodeNamesL	= { "SM_Blade_L" };
	constexpr float kBladeBaseOffset		= 0.1f;		// 手から刃の根元（つば）までの距離

	// 剣の軌跡
	constexpr float kSwordTrailWidthRate	= 1.0f;		// 帯の幅の倍率（1.0で刃の根元から先端まで）
	constexpr UINT	kSwordTrailLength		= 15;		// 軌跡を残すフレーム数
	constexpr int	kSwordTrailSubdivision	= 6;		// 1フレームの間を何分割して補間するか（多いほどなめらか）

	// 軌跡テクスチャ（横：刃の先端→根元、縦：新しい→古い）
	constexpr int	kSwordTrailTexWidth		= 64;
	constexpr int	kSwordTrailTexHeight	= 256;
	const Math::Vector3 kSwordTrailColor	= { 0.25f, 0.65f, 1.0f };	// 軌跡の色
	const Math::Vector3 kSwordTrailCoreColor = { 1.0f, 1.0f, 1.0f };	// 刃先付近の芯の色
	constexpr float kSwordTrailBaseBrightness = 0.15f;	// 刃の根元側の明るさ（先端を1.0とした割合）

	// エフェクト
	constexpr float kHitEffectScale			= 0.5f;		// 攻撃が敵に当たった時のエフェクトの大きさ
	constexpr float kCriticalHitEffectScale	= 1.0f;		// クリティカル時のエフェクトの大きさ
	constexpr float kAwakeningEffectScale	= 1.0f;		// 覚醒オーラの大きさ

	// カメラの注視点を腰の骨から下げる量
	constexpr float kCameraTargetOffsetY	= -0.9f;
	constexpr float kCameraFollowRateY		= 0.25f;	// ジャンプなどで高さが変わった時、カメラが付いていく速さ（0～1）
	constexpr float kCameraRiseDeadZone		= 0.15f;	// 腰がこれ以上上がった時だけカメラも上げる（攻撃の小さな上下では動かさない）

	// モデル・アニメーション（UEでSK_Assassinにリターゲットして書き出したもの）
	const std::string kPlayerAssetDir		= "Asset/Models/GameObject/Player/New/";
	const std::string kPlayerModelPath		= kPlayerAssetDir + "CharaBase/SK_Assassin.gltf";
	const std::string kAnimDir				= kPlayerAssetDir + "Sequence2/";

	// 武器のモデル（空文字なら読み込まない）
	// ※SK_Assassin には剣（SM_Blade）が最初から含まれているので、別の剣は読み込まない
	const std::string kSwordModelPath		= "";

	// 武器を付けるノード（上から順に探して、最初に見つかったものを使う）
	const std::vector<std::string> kSwordNodeNamesR		= { "Blade_R", "Weapon_R", "hand_r" };
	const std::vector<std::string> kSwordNodeNamesL		= { "Blade_L", "Weapon_L", "hand_l" };

	// 剣を投げる攻撃（投げた剣がまっすぐ前へ飛ぶように補正する）
	const std::string kThrowAnimName		= "Attack_5_2";
	constexpr float kThrowCorrectStartDist	= 0.3f;		// 手からこの距離までは補正しない（手に持っている間）
	constexpr float kThrowCorrectFullDist	= 1.0f;		// 手からこの距離以上離れたら、補正を全部かける

	// 着地アニメーション（両足の接地フレームを事前に計算する）
	// ※プレイヤーは常に剣を抜いているので、剣を持った（Combat_）アニメーションだけを使う
	const std::vector<std::string> kLandingAnimNames	= { "Combat_JumpEnd" };
	const std::vector<std::string> kTakeoffAnimNames	= { "Combat_JumpStart" };
	constexpr float kMaxTakeoffFrame		= 15.0f;	// 踏み切りがこれより遅いアニメーションは、待たずにすぐ跳ぶ（操作の遅れを防ぐ）

	// 溜め攻撃（4-3）の最後で、剣を地面に叩きつけるアニメーション
	const std::string kSlamAnimName			= "Attack_4_3_End";
	constexpr float kSwordSlamTolerance		= 0.05f;	// 剣先が一番低い高さから、これ以内に来たら「叩きつけた」とみなす
	const std::vector<std::string> kFootNodeNames		= { "foot_l", "foot_r" };
}

// 読み込むアニメーションのリストを作成
// ※プレイヤーは常に剣を抜いているので、剣を納めている時のアニメーション（Idle・Run・Jump・抜刀・納刀・ダッシュ）は読み込まない
//   （ファイルは Asset フォルダに残してある）
std::vector<AnimLoadInfo> Player::CreateAnimList()
{
	std::vector<AnimLoadInfo> animList =
	{
		{ "Combat_Idle",		kAnimDir + "01_Idle/02_Idle_Combat/AS_Idle_Combat_Seq/AS_Idle_Combat_Seq.gltf" },
		{ "Evade",				kAnimDir + "07_Roll/02_Roll_Combat/AS_Roll_Combat_F_0_Seq/AS_Roll_Combat_F_0_Seq.gltf" },
		{ "Combat_Run",			kAnimDir + "04_Run/02_Run_Combat/01_Run_Combat_F_0/AS_Run_Combat_F_0_Loop_Seq/AS_Run_Combat_F_0_Loop_Seq.gltf" },
		{ "Combat_JumpStart",	kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_Start_0_Seq/AS_Jump_Combat_Start_0_Seq.gltf" },
		{ "Combat_JumpLoop",	kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_Loop_0_Seq/AS_Jump_Combat_Loop_0_Seq.gltf" },
		{ "Combat_JumpEnd",		kAnimDir + "05_Jump/02_Jump_Combat/AS_Jump_Combat_End_0_Seq/AS_Jump_Combat_End_0_Seq.gltf" },
		{ "Damage",				kAnimDir + "08_Hit/01_Hit/AS_Hit_F_Seq/AS_Hit_F_Seq.gltf" },
		{ "Death",				kAnimDir + "08_Hit/01_Hit/AS_Hit_Death_Seq/AS_Hit_Death_Seq.gltf" },
	};

	// 攻撃アニメーション（Attack_タイプ_段数）
	for (int type = 1; type <= kAttackTypeNum; ++type)
	{
		for (int step = 1; step <= 4; ++step)
		{
			std::string typeStr = "0" + std::to_string(type);
			std::string stepStr = "0" + std::to_string(step);

			// 4-3 だけは Start / Loop / End の3分割
			if (type == 4 && step == 3)
			{
				std::string basePath = kAnimDir + "02_Attack/04_Combo_Attack_04/AS_Combo_Attack_04_03_";
				animList.push_back({ "Attack_4_3_Start", basePath + "Start_Seq/AS_Combo_Attack_04_03_Start_Seq.gltf" });
				animList.push_back({ "Attack_4_3_Loop",  basePath + "Loop_Seq/AS_Combo_Attack_04_03_Loop_Seq.gltf" });
				animList.push_back({ "Attack_4_3_End",   basePath + "End_Seq/AS_Combo_Attack_04_03_End_Seq.gltf" });
				continue;
			}

			std::string animName = "Attack_" + std::to_string(type) + "_" + std::to_string(step);
			std::string fileName = "AS_Combo_Attack_" + typeStr + "_" + stepStr + "_Seq";
			std::string path = kAnimDir + "02_Attack/"
				+ typeStr + "_Combo_Attack_" + typeStr + "/"
				+ fileName + "/" + fileName + ".gltf";

			animList.push_back({ animName, path });
		}
	}

	return animList;
}

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
	if (!m_model || shape.nodeIndex < 0 || shape.nodeIndex >= static_cast<int>(m_model->GetNodes().size()))
	{
		return Math::Matrix::Identity;
	}

	// 剣のメッシュが今いる位置（モデルの中の位置 × キャラの位置）
	Math::Matrix nodeMat = m_model->GetNodes()[shape.nodeIndex].m_worldTransform * m_mWorld;

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

std::shared_ptr<KdTexture> Player::CreateSwordTrailTexture()
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
			Math::Vector3 color = Math::Vector3::Lerp(kSwordTrailColor, kSwordTrailCoreColor, core) * intensity;

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

void Player::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_swordModel = std::make_shared<KdModelData>();
	m_swordTrailR.polygon = std::make_shared<KdTrailPolygon>();
	m_swordTrailL.polygon = std::make_shared<KdTrailPolygon>();

	// 当たり判定
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape
	(
		"Player",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);

	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// モデル・武器セットの読み込み
	m_model->SetModelData(kPlayerModelPath);

	// アニメーションで腰が水平に動いた分は、プレイヤー自体の移動に置き換える
	// （体が元の位置に戻らず、アニメーションが自然につながるようにする）
	SetExtractPelvisMotion(true);
	if (!kSwordModelPath.empty())		m_swordModel->Load(kSwordModelPath);

	// 武器をどのノードに付けたかを出力ウィンドウに表示（モデルを差し替えた時の確認用）
	for (const auto* pNames : { &kSwordNodeNamesR, &kSwordNodeNamesL })
	{
		const KdModelWork::Node* pNode = FindFirstNode(*pNames);
		OutputDebugStringA(("武器の取り付け先: " + (pNode ? pNode->m_name : std::string("見つからない")) + "\n").c_str());
	}

	PlayerParamManager::Instance().Load("Asset/Data/PlayerParams.txt");
	PlayerParamManager::Instance().LoadWeaponParams("Asset/Data/WeaponParams.json");

	// モデルに剣が含まれていれば、剣のメッシュから刃の向き・長さを求める
	// （攻撃判定の箱と軌跡を、実際の刃に合わせるため）
	m_swordBladeBaseY = kSwordBladeBaseY;
	m_swordBladeTipY = kSwordBladeTipY;
	if (m_model->GetData())
	{
		m_bladeShapeR = CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesR);
		m_bladeShapeL = CalcBladeShape(*m_model->GetData(), kBladeMeshNodeNamesL);
	}
	if (m_bladeShapeR.nodeIndex >= 0)
	{
		m_swordBladeBaseY = std::min(kBladeBaseOffset, m_bladeShapeR.tipLength * 0.5f);
		m_swordBladeTipY = m_bladeShapeR.tipLength;
		OutputDebugStringA(("剣のメッシュから刃の長さを計算: " + std::to_string(m_bladeShapeR.tipLength) + "\n").c_str());
	}

	// アニメーションの読み込み
	LoadAnimations(CreateAnimList());

	// 剣を投げる攻撃で、剣をまっすぐ前へ向けるための角度を事前に計算
	m_weaponNodeR = FindNodeIndex("weapon_r");
	m_weaponNodeL = FindNodeIndex("weapon_l");
	m_handNodeR = FindNodeIndex("hand_r");
	m_handNodeL = FindNodeIndex("hand_l");
	m_pelvisNode = FindNodeIndex("pelvis");
	m_throwAngleR = CalcThrowCorrectionAngle(kThrowAnimName, m_weaponNodeR);
	m_throwAngleL = CalcThrowCorrectionAngle(kThrowAnimName, m_weaponNodeL);
	OutputDebugStringA(("剣を投げる向きの補正: 右 " + std::to_string(m_throwAngleR)
		+ " 度 / 左 " + std::to_string(m_throwAngleL) + " 度\n").c_str());

	// ジャンプ開始アニメーションで、足が地面から離れる（踏み切る）フレームを事前に計算
	for (const auto& animName : kTakeoffAnimNames)
	{
		if (!HasAnimation(animName)) continue;

		float frame = CalcFootLiftFrame(animName, kFootNodeNames);
		m_takeoffFrames[animName] = (frame >= 0.0f && frame <= kMaxTakeoffFrame) ? frame : 0.0f;
		OutputDebugStringA(("踏み切りフレーム: " + animName + " = " + std::to_string(frame) + "\n").c_str());
	}

	// ジャンプのアニメーションで、腰がどれだけ上下するかを出力ウィンドウに表示（ジャンプの調整用）
	// （上がる量が大きいほど、アニメーション自体に「跳ぶ高さ」が入っている）
	for (const char* animName : { "Combat_JumpStart", "Combat_JumpLoop", "Combat_JumpEnd" })
	{
		auto it = m_animMap.find(animName);
		if (it == m_animMap.end() || !it->second) continue;

		float minY = std::numeric_limits<float>::max();
		float maxY = -std::numeric_limits<float>::max();
		for (float t = 0.0f; t <= it->second->m_maxLength; t += 1.0f)
		{
			Math::Vector3 pelvisPos;
			SampleMotionPos(*it->second, t, &pelvisPos);
			minY = std::min(minY, pelvisPos.y - m_pelvisRestPos.y);
			maxY = std::max(maxY, pelvisPos.y - m_pelvisRestPos.y);
		}
		OutputDebugStringA(("ジャンプの腰の上下: " + std::string(animName)
			+ " 長さ " + std::to_string(it->second->m_maxLength)
			+ " / 最低 " + std::to_string(minY) + " / 最高 " + std::to_string(maxY) + "\n").c_str());
	}

	// 溜め攻撃（4-3）で、剣を地面に叩きつけるフレームを事前に計算（軌跡をそこまで出すため）
	m_swordSlamFrame = CalcSwordLowestFrame(kSlamAnimName);
	OutputDebugStringA(("剣を叩きつけるフレーム: " + std::to_string(m_swordSlamFrame) + "\n").c_str());

	// 着地アニメーションで両足が接地するフレームを事前に計算
	for (const auto& animName : kLandingAnimNames)
	{
		if (!HasAnimation(animName)) continue;

		float frame = CalcFootPlantFrame(animName, kFootNodeNames);
		m_landingFrames[animName] = frame;
		OutputDebugStringA(("着地フレーム: " + animName + " = " + std::to_string(frame) + "\n").c_str());
	}

	ChangeState(std::make_shared<PlayerStateIdle>());
	ChangeAnimation("Combat_Idle", true);

	// 剣の軌跡（左右）
	m_swordTrailTex = CreateSwordTrailTexture();
	for (SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		pTrail->polygon->SetMaterial(m_swordTrailTex);
		pTrail->polygon->SetLength(kSwordTrailLength * kSwordTrailSubdivision);
	}

	m_pos = { 0, 0.0f, 0.5f };
	m_prevFramePos = m_pos;

	// カメラの注視点の高さ：立っている時の腰の高さを基準にする（腰が上下に動いても付いていかない）
	float pelvisHeight = (m_pelvisRestPos.y > 0.0f) ? m_pelvisRestPos.y : 0.9f;	// 腰の骨が無いモデルの時は標準の高さ
	m_cameraHeight = pelvisHeight + kCameraTargetOffsetY;
	m_cameraTargetY = m_pos.y + m_cameraHeight;

	m_maxHp = kMaxHp;
	m_hp = m_maxHp;

	// HPゲージ
	std::shared_ptr<HPGage> hpGage = std::make_shared<HPGage>();
	hpGage->Init();
	if (m_owner)
	{
		m_owner->AddObject(hpGage);
	}
	m_wpHpGage = hpGage;

	// ATBゲージ（HPゲージの下）
	std::shared_ptr<ATBGage> atbGage = std::make_shared<ATBGage>();
	atbGage->Init();
	if (m_owner)
	{
		m_owner->AddObject(atbGage);
	}
	m_wpAtbGage = atbGage;

	// 最初はオペレーターモード
	m_atbGage = 0.0f;
	SetBattleMode(BattleMode::Operator);
}

void Player::Update()
{
	// ヒットストップ中はアニメーションも移動も止め、座標の更新（描画）だけ行う
	if (m_hitStopTimer > 0)
	{
		m_hitStopTimer--;
		UpdateWorldMatrix();
		return;
	}

	// 前のフレームに水平方向へ動いた量を記録（ジャンプで勢いを引き継ぐため）
	m_lastMoveDelta = m_pos - m_prevFramePos;
	m_lastMoveDelta.y = 0.0f;
	m_prevFramePos = m_pos;

	// ルートモーションの切り替え（確認用。オフにすると、腰の動きはその場でのアニメーションになる）
	if (m_extractPelvisMotion != s_useRootMotion)
	{
		SetExtractPelvisMotion(s_useRootMotion);
	}

	// アニメーション時間の進行とCalcNodeMatricesの実行はBaseで
	BaseChara::Update();

	// アニメーションで腰が動いた分だけ、プレイヤーを動かす
	ApplyAnimationMotion();

	// 剣を投げる攻撃では、投げた剣がまっすぐ前へ飛ぶように補正する
	ApplyThrowCorrection();

	UpdateInvincibleTimer();

	m_isTrailAddedThisFrame = false;

	if (m_state)
	{
		m_state->Update(this);
	}

	UpdateSwordTrails();
	UpdateAwakening();
	UpdateHpGage();
	UpdateModeSwitch();
	UpdateAttackTypeSwitch();
	UpdateAtb();

	UpdateWorldMatrix();
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

void Player::ApplyAnimationMotion()
{
	if (!m_useRootMotion) return;

	// 腰の移動量はモデルから見た向きなので、モデルの向き（m_angle + 180度）で回してワールドの向きにする
	Math::Matrix modelRot = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
	Math::Vector3 move = Math::Vector3::TransformNormal(m_rootMoveDelta, modelRot) * m_rootScale;

	// 上下の移動は重力と地面判定に任せる
	move.y = 0.0f;
	m_pos += move;
}

void Player::PostUpdate()
{
	BaseChara::PostUpdate();
	UpdateWeaponMatrix();
	UpdateCameraTargetY();
}

void Player::UpdateCameraTargetY()
{
	// 基本は足元の高さに合わせる（ジャンプ・階段・落下）
	float desiredY = m_pos.y + m_cameraHeight;

	// アニメーションで体が上に上がった分（ジャンプや飛び上がる攻撃）だけ、カメラも上げる
	// 腰が下がった分（回避・しゃがむ動き）は無視するので、回避でカメラがガクンと下がらない
	if (m_model && m_pelvisNode >= 0 && m_pelvisNode < static_cast<int>(m_model->GetNodes().size()))
	{
		float pelvisY = m_model->GetNodes()[m_pelvisNode].m_worldTransform.Translation().y;
		float rise = pelvisY - m_pelvisRestPos.y - kCameraRiseDeadZone;
		if (rise > 0.0f) desiredY += rise;
	}

	m_cameraTargetY += (desiredY - m_cameraTargetY) * kCameraFollowRateY;
}

void Player::GenerateDepthMapFromLight()
{
}

void Player::DrawEffect()
{
	auto& shaderManager = KdShaderManager::Instance();

	// 軌跡は他のオブジェクトを描いた後に、加算合成・深度書き込みなしで描く
	// （帯の暗い部分が後ろの物を隠さないように）
	// 裏からも見えるようにカリングも切る
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	for (const SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		if (pTrail->polygon)
		{
			shaderManager.m_StandardShader.DrawPolygon(*pTrail->polygon);
		}
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

void Player::DrawLit()
{
	if (!m_model) return;

	auto& shaderManager = KdShaderManager::Instance();
	auto& shader = shaderManager.m_StandardShader;

	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Alpha);

	shader.DrawModel(*m_model, m_mWorld);

	if (m_swordModel)
	{
		shader.DrawModel(*m_swordModel, m_swordWorldR);
		shader.DrawModel(*m_swordModel, m_swordWorldL);
	}

	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

void Player::UpdateWorldMatrix()
{
	// モデルが逆向きなので180度回して描画する
	Math::Matrix rotate = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = rotate * trans;
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

void Player::UpdateAwakening()
{
	if (m_isAwakening)
	{
		m_awakeningTimer--;
		if (m_awakeningTimer <= 0) m_isAwakening = false;
	}

	UpdateAwakeningEffect();
}

void Player::UpdateAwakeningEffect()
{
	if (!m_isAwakening)
	{
		EffectPlayer::Stop(m_awakeningEffect);
		return;
	}

	// 再生が終わっていたら再生し直し、再生中はプレイヤーの位置に追従させる
	if (!m_awakeningEffect || !m_awakeningEffect->IsPlaying())
	{
		m_awakeningEffect = EffectPlayer::Play(EffectName::AwakeningAura, m_pos, kAwakeningEffectScale);
	}
	else
	{
		m_awakeningEffect->SetPos(m_pos);
	}
}

Math::Matrix Player::CreateEffectMatrix(float forwardOffset, float height, float scale) const
{
	// m_angle の向きが前方（ローカル+Z）になる
	Math::Matrix rot = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), rot);
	Math::Vector3 pos = m_pos + forward * forwardOffset + Math::Vector3(0.0f, height, 0.0f);

	return Math::Matrix::CreateScale(scale) * rot * Math::Matrix::CreateTranslation(pos);
}

void Player::UpdateHpGage()
{
	auto hpGage = m_wpHpGage.lock();
	if (!hpGage) return;

	float ratio = static_cast<float>(m_hp) / static_cast<float>(m_maxHp);
	hpGage->SetHpRatio(std::max(ratio, 0.0f));
}

const std::vector<int>& Player::GetModeAttackTypes(BattleMode mode)
{
	return (mode == BattleMode::Punisher) ? kPunisherAttackTypes : kOperatorAttackTypes;
}

void Player::UpdateAttackTypeSwitch()
{
	// Cキーで攻撃パターンを切り替え（今のモードで使える攻撃パターンの中をループ）
	if (!m_attackTypeKey.Update()) return;

	const std::vector<int>& types = GetModeAttackTypes(m_battleMode);
	int& index = m_modeAttackTypeIndex[static_cast<int>(m_battleMode)];
	index = (index + 1) % static_cast<int>(types.size());

	m_currentAttackType = std::clamp(types[index], 1, kAttackTypeNum);
	s_currentAttackType = m_currentAttackType;

	KdDebugGUI::Instance().AddLog
	(
		"[System] 攻撃タイプ切り替え: 0%d\n",
		m_currentAttackType
	);
}

void Player::SetBattleMode(BattleMode mode)
{
	m_battleMode = mode;

	// 攻撃パターンを、そのモードで最後に選んでいたものにする
	const std::vector<int>& types = GetModeAttackTypes(mode);
	int& index = m_modeAttackTypeIndex[static_cast<int>(mode)];
	index = std::clamp(index, 0, static_cast<int>(types.size()) - 1);
	m_currentAttackType = std::clamp(types[index], 1, kAttackTypeNum);
	s_currentAttackType = m_currentAttackType;

	// 画面のモード表示を変える
	if (auto atbGage = m_wpAtbGage.lock())
	{
		atbGage->SetPunisherMode(mode == BattleMode::Punisher);
	}
}

float Player::GetModeMoveRate() const
{
	return (m_battleMode == BattleMode::Punisher) ? kPunisherMoveRate : 1.0f;
}

void Player::UpdateModeSwitch()
{
	// Eキーが押されたら、オペレーター ⇔ パニッシャー を切り替える
	// （キーの状態は毎フレーム更新する。倒れている時は切り替えない）
	if (!m_modeKey.Update() || IsDead()) return;

	bool toPunisher = (m_battleMode == BattleMode::Operator);
	SetBattleMode(toPunisher ? BattleMode::Punisher : BattleMode::Operator);

	// 切り替えた合図のエフェクト（足元）
	EffectPlayer::Play(EffectName::ModeChange, m_pos, 1.0f);

	KdDebugGUI::Instance().AddLog
	(
		"[System] モード切り替え: %s\n",
		toPunisher ? "PUNISHER" : "OPERATOR"
	);
}

void Player::AddAtbGage(float val)
{
	m_atbGage = std::clamp(m_atbGage + val, 0.0f, kAtbBarMax * kAtbBarNum);
}

bool Player::UseAtbBar()
{
	if (m_atbGage < kAtbBarMax) return false;

	m_atbGage -= kAtbBarMax;
	return true;
}

void Player::UpdateAtb()
{
	// オペレーターの時は、時間でも少しずつたまる
	if (m_battleMode == BattleMode::Operator && !IsDead())
	{
		AddAtbGage(kAtbPassiveGain);
	}

	// UIへ「何本分たまっているか」を伝える（例：1.5 なら 1本目が満タン、2本目が半分）
	if (auto atbGage = m_wpAtbGage.lock())
	{
		atbGage->SetAtbValue(m_atbGage / kAtbBarMax);
	}
}

void Player::UpdateAwakeningInput()
{
	// Uキーでゲージを満タンにする（デバッグ用）
	if (IsKeyDown('U'))
	{
		m_awakeningGage = m_maxAwakeningGage;
	}

	// Gキーで覚醒発動！
	if (IsKeyDown('G') && m_awakeningGage >= m_maxAwakeningGage && !m_isAwakening)
	{
		m_isAwakening = true;
		m_awakeningGage = 0.0f;
		m_awakeningTimer = kAwakeningFrame;
		m_hp = std::min(m_hp + static_cast<int>(m_maxHp * kAwakeningHealRate), m_maxHp);
	}
}

void Player::AddAwakeningGage(float val)
{
	m_awakeningGage = std::min(m_awakeningGage + val, m_maxAwakeningGage);
}

void Player::Release()
{
	m_model = nullptr;
	m_swordModel = nullptr;
}

void Player::ChangeState(const std::shared_ptr<PlayerState>& newState)
{
	// アビリティの攻撃中の印は、次のステートに入る時に消す（アビリティのステートが入る時にまた付ける）
	m_isAbilityAttack = false;

	// ジャンプ用の設定（空中で腰が上がらないようにする・速く落ちる）も、ステートが変わったら元に戻す
	SetAirPelvisLock(false);
	SetFallGravityRate(1.0f);

	m_state = newState;
	if (m_state)
	{
		m_state->ChangeState(this);
	}
}

Math::Vector3 Player::GetInputDir() const
{
	Math::Vector3 dir = Math::Vector3::Zero;

	if (IsKeyDown('W')) dir.z += 1.0f;
	if (IsKeyDown('S')) dir.z -= 1.0f;
	if (IsKeyDown('A')) dir.x -= 1.0f;
	if (IsKeyDown('D')) dir.x += 1.0f;

	return dir;
}

bool Player::CheckMoveInput() const
{
	return GetInputDir().LengthSquared() > 0.0f;
}

bool Player::IsOnGround() const
{
	return m_gravity >= 0.0f && m_gravity <= kOnGroundGravityMax;
}

float Player::GetTakeoffFrame(const std::string& animName) const
{
	auto it = m_takeoffFrames.find(animName);
	return (it != m_takeoffFrames.end()) ? it->second : 0.0f;
}

float Player::CalcSwordLowestFrame(const std::string& animName) const
{
	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !m_model || !m_model->GetData()) return -1.0f;

	// 剣先の位置（剣のメッシュから求めた刃の形を使う）
	std::vector<const BladeShape*> blades;
	for (const BladeShape* pShape : { &m_bladeShapeR, &m_bladeShapeL })
	{
		if (pShape->nodeIndex >= 0) blades.push_back(pShape);
	}
	if (blades.empty()) return -1.0f;

	// 計算用にモデルを複製し、1フレームずつ再生して、左右の剣先の低い方の高さを記録する
	KdModelWork work(m_model->GetData());
	KdAnimator animator;
	animator.SetAnimation(it->second, false);

	std::vector<float> times;
	std::vector<float> heights;
	for (int i = 0; i < 1000 && !animator.IsAnimationEnd(); ++i)
	{
		times.push_back(animator.GetTime());
		animator.AdvanceTime(work.WorkNodes(), 1.0f);
		work.CalcNodeMatrices();

		float lowest = std::numeric_limits<float>::max();
		for (const BladeShape* pShape : blades)
		{
			Math::Vector3 localTip = pShape->localHand + pShape->localDir * pShape->tipLength;
			Math::Vector3 tip = Math::Vector3::Transform(localTip, work.GetNodes()[pShape->nodeIndex].m_worldTransform);
			lowest = std::min(lowest, tip.y);
		}
		heights.push_back(lowest);
	}
	if (heights.empty()) return -1.0f;

	// 一番低い高さに最初に近づいたフレーム（＝地面に叩きつけた瞬間）
	float minHeight = *std::min_element(heights.begin(), heights.end());
	for (size_t i = 0; i < heights.size(); ++i)
	{
		if (heights[i] <= minHeight + kSwordSlamTolerance) return times[i];
	}
	return -1.0f;
}

float Player::GetLandingFrame(const std::string& animName) const
{
	auto it = m_landingFrames.find(animName);
	return (it != m_landingFrames.end()) ? it->second : 0.0f;
}

void Player::ChangeAnimationLazy(const std::string& animName, bool isLoop, bool forceRestart, float blendFrame)
{
	// パスが登録されていれば、その場でロードしてマップに登録する
	auto it = m_lazyAnimPaths.find(animName);
	if (it != m_lazyAnimPaths.end())
	{
		KdModelWork tempModel;
		tempModel.SetModelData(it->second);
		if (auto anim = tempModel.GetAnimation(0))
		{
			SetAnimationData(animName, anim);
			OutputDebugStringA(("遅延ロード完了: " + animName + "\n").c_str());
		}
	}

	// 登録済みの通常のアニメーション変更を呼び出す
	ChangeAnimation(animName, isLoop, forceRestart, blendFrame);
}

void Player::PlayHitEffect(const std::string& effectName, const Math::Vector3& hitPos, bool isCritical) const
{
	// 攻撃の種類ごとのエフェクトが無ければ、いつものヒットエフェクト
	const std::string& name = effectName.empty() ? EffectName::HitEnemy : effectName;

	// プレイヤーの攻撃の向きに合わせ、当たった位置に出す（クリティカルは大きく）
	float scale = isCritical ? kCriticalHitEffectScale : kHitEffectScale;
	Math::Matrix mWorld = Math::Matrix::CreateScale(scale)
		* Math::Matrix::CreateRotationY(GetAngleRad())
		* Math::Matrix::CreateTranslation(hitPos);
	EffectPlayer::Play(name, mWorld);
}

bool Player::AttackOBB(const Math::Matrix& swordMatrix, bool isCritical, const std::string& hitEffectName, float damageRate)
{
	// 刃の根元から先端までを囲む、Y軸方向に長い箱
	float obbOffsetY = (m_swordBladeBaseY + m_swordBladeTipY) * 0.5f;
	Math::Vector3 obbExtents = { kSwordOBBHalfWidth, (m_swordBladeTipY - m_swordBladeBaseY) * 0.5f, kSwordOBBHalfWidth };
	Math::Matrix obbMatrix = Math::Matrix::CreateTranslation(0.0f, obbOffsetY, 0.0f) * swordMatrix;
	Math::Vector3 offset = Math::Vector3::Zero;

	KdCollider::BoxInfo obbInfo(KdCollider::TypeDamage, obbMatrix, offset, obbExtents, true);

	// デバッグ用の赤い箱を描画
	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugBox(obbMatrix, obbExtents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	bool isHit = false;
	// ダメージ（パニッシャーは重い攻撃なので倍率をかける）
	float modeRate = (m_battleMode == BattleMode::Punisher) ? kPunisherDamageRate : 1.0f;
	int baseDamage = isCritical ? kCriticalAttackDamage : kAttackDamage;
	int damage = static_cast<int>(std::round(baseDamage * modeRate * damageRate));

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;
		if (!obj->Intersects(obbInfo, &retList)) continue;

		// 敵以外・無敵中・倒れている敵には何もしない
		// （毎フレームヒット扱いになって、ヒットストップやエフェクトが連続しないように）
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || enemy->IsInvincible() || !enemy->IsAlive()) continue;

		enemy->OnDamage(damage, isCritical);
		isHit = true;

		// 攻撃を当てるとATBゲージがたまる（パニッシャーは多くたまる。アビリティの攻撃ではたまらない）
		if (!m_isAbilityAttack)
		{
			float gain = kAtbHitGain;
			if (m_battleMode == BattleMode::Punisher) gain *= kPunisherAtbHitRate;
			if (isCritical) gain *= kAtbCriticalRate;
			AddAtbGage(gain);
		}

		// 当たった位置にヒットエフェクト
		// （敵は当たると少しの間無敵になるので、同じ敵に毎フレーム出続けることはない）
		Math::Vector3 hitPos = retList.empty() ? obbMatrix.Translation() : retList.back().m_hitPos;
		PlayHitEffect(hitEffectName, hitPos, isCritical);
	}

	return isHit;
}

bool Player::GetMoveInputDir(Math::Vector3& outDir) const
{
	// カメラの向きを基準に移動方向を決める
	Math::Matrix cameraRotY = Math::Matrix::Identity;
	if (auto camera = m_camera.lock())
	{
		cameraRotY = camera->GetRotationYMatrix();
	}

	outDir = Math::Vector3::TransformNormal(GetInputDir(), cameraRotY);
	outDir.y = 0.0f;
	if (outDir.LengthSquared() == 0.0f) return false;

	outDir.Normalize();
	return true;
}

bool Player::MoveProcess(bool moveByCode)
{
	UpdateAwakeningInput();

	Math::Vector3 dir;
	if (!GetMoveInputDir(dir)) return false;

	// 旋回（1フレームに最大kMaxTurnAngle度まで）
	TurnToward(dir, kMaxTurnAngle);

	// 座標移動（ルートモーションで動く時は、ここでは動かさない）
	if (moveByCode)
	{
		float moveSpeed = (m_isAwakening ? kAwakeningMoveSpeed : kMoveSpeed) * GetModeMoveRate();
		m_pos += dir * moveSpeed;
	}
	return true;
}

void Player::TurnToward(const Math::Vector3& dir, float maxAngle)
{
	Math::Matrix nowRot = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 nowDir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), nowRot);
	float angle = DirectX::XMConvertToDegrees(acos(std::clamp(nowDir.Dot(dir), -1.0f, 1.0f)));

	if (angle < kMinTurnAngle) return;

	angle = std::min(angle, maxAngle);
	if (nowDir.Cross(dir).y >= 0) { m_angle += angle; }
	else { m_angle -= angle; }
}

void Player::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;
	m_invincibleTimer = kDamageInvincibleFrame;

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<PlayerStateDead>());
	}
	else if (isCritical || damage >= kHeavyDamage)
	{
		ChangeState(std::make_shared<PlayerStateDamage>());
	}
}

Math::Vector3 Player::GetSwordTipPositionR() const
{
	return GetSwordPoint(m_swordWorldR, m_swordBladeTipY);
}

Math::Vector3 Player::GetSwordTipPositionL() const
{
	return GetSwordPoint(m_swordWorldL, m_swordBladeTipY);
}

Math::Vector3 Player::GetSwordBasePositionR() const
{
	return GetSwordPoint(m_swordWorldR, m_swordBladeBaseY);
}

Math::Vector3 Player::GetSwordBasePositionL() const
{
	return GetSwordPoint(m_swordWorldL, m_swordBladeBaseY);
}

Math::Vector3 Player::GetCameraTargetPos() const
{
	// X・Z は足元の位置、Y は回避やジャンプで上下しない高さを使う
	// （前は腰の骨の高さを使っていたため、回避で腰が下がるとカメラがガクンと下がっていた）
	Math::Vector3 targetPos = m_pos;
	targetPos.y = m_cameraTargetY;
	return targetPos;
}
