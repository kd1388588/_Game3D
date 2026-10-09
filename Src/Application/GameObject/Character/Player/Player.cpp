// Player.cpp

// Player
#include "Player.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"

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

Math::Vector3 Player::s_sheathedRotR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedPosR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedRotL = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedPosL = { 0.0f, 0.0f, 0.0f };

int Player::s_currentAttackType = 1;
float Player::s_attackRootScale = 0.2f;

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

	// 覚醒
	constexpr int	kAwakeningFrame			= 60 * 15;	// 15秒間持続
	constexpr float	kAwakeningHealRate		= 0.3f;		// HP3割回復

	// 接地判定に使う重力の上限（落下し始めの1フレーム分）
	constexpr float kOnGroundGravityMax		= 0.011f;

	// 剣の刃の位置（剣のローカルY軸上。攻撃判定の箱と同じ範囲）
	constexpr float kSwordBladeBaseY		= 0.05f;	// 刃の根元
	constexpr float kSwordBladeTipY			= 1.15f;	// 刃の先端
	constexpr float kSwordOBBOffsetY		= (kSwordBladeBaseY + kSwordBladeTipY) * 0.5f;	// 判定の箱の中心
	const Math::Vector3 kSwordOBBExtents	= { 0.1f, (kSwordBladeTipY - kSwordBladeBaseY) * 0.5f, 0.1f };	// Y軸方向に長い箱

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

	// モデル・アニメーション（UEでSK_Assassinにリターゲットして書き出したもの）
	const std::string kPlayerAssetDir		= "Asset/Models/GameObject/Player/New/";
	const std::string kPlayerModelPath		= kPlayerAssetDir + "CharaBase/SK_Assassin.gltf";
	const std::string kAnimDir				= kPlayerAssetDir + "Sequence2/";

	// 武器のモデル（空文字なら読み込まない）
	const std::string kSwordModelPath		= "Asset/Models/GameObject/Player/Sword/Kari/sword.gltf";
	const std::string kScabbardModelPath	= "";

	// 武器・鞘を付けるノード（上から順に探して、最初に見つかったものを使う）
	const std::vector<std::string> kSwordNodeNamesR		= { "Blade_R", "Weapon_R", "hand_r" };
	const std::vector<std::string> kSwordNodeNamesL		= { "Blade_L", "Weapon_L", "hand_l" };
	const std::vector<std::string> kScabbardNodeNamesR	= { "Weapon_Holder_R", "Scabbard_R", "spine_05" };
	const std::vector<std::string> kScabbardNodeNamesL	= { "Weapon_Holder_L", "Scabbard_L", "spine_05" };

	// 武器装備中のアニメーション名に付ける接頭辞
	const std::string kCombatAnimPrefix = "Combat_";

	// 着地アニメーション（両足の接地フレームを事前に計算する）
	const std::vector<std::string> kLandingAnimNames	= { "JumpEnd", "Combat_JumpEnd" };
	const std::vector<std::string> kFootNodeNames		= { "foot_l", "foot_r" };
}

// 読み込むアニメーションのリストを作成
std::vector<AnimLoadInfo> Player::CreateAnimList()
{
	std::vector<AnimLoadInfo> animList =
	{
		{ "Idle",				kAnimDir + "01_Idle/01_Idle/AS_Idle_Seq/AS_Idle_Seq.gltf" },
		{ "Equip",				kAnimDir + "01_Idle/01_Idle/AS_Idle_to_Idle_Combat_Seq/AS_Idle_to_Idle_Combat_Seq.gltf" },
		{ "Unequip",			kAnimDir + "01_Idle/02_Idle_Combat/AS_Idle_Combat_to_Idle_Seq/AS_Idle_Combat_to_Idle_Seq.gltf" },
		{ "Combat_Idle",		kAnimDir + "01_Idle/02_Idle_Combat/AS_Idle_Combat_Seq/AS_Idle_Combat_Seq.gltf" },
		{ "Evade",				kAnimDir + "07_Roll/02_Roll_Combat/AS_Roll_Combat_F_0_Seq/AS_Roll_Combat_F_0_Seq.gltf" },
		{ "Dash",				kAnimDir + "06_Dodge/01_Dodge/AS_Dodge_F_0_Seq/AS_Dodge_F_0_Seq.gltf" },
		{ "Combat_Run",			kAnimDir + "04_Run/02_Run_Combat/01_Run_Combat_F_0/AS_Run_Combat_F_0_Loop_Seq/AS_Run_Combat_F_0_Loop_Seq.gltf" },
		{ "Run",				kAnimDir + "04_Run/01_Run/01_Run_F_0/AS_Run_F_0_Loop_Seq/AS_Run_F_0_Loop_Seq.gltf" },
		{ "JumpStart",			kAnimDir + "05_Jump/01_Jump/AS_Jump_Start_0_Seq/AS_Jump_Start_0_Seq.gltf" },
		{ "JumpLoop",			kAnimDir + "05_Jump/01_Jump/AS_Jump_Loop_0_Seq/AS_Jump_Loop_0_Seq.gltf" },
		{ "JumpEnd",			kAnimDir + "05_Jump/01_Jump/AS_Jump_End_0_Seq/AS_Jump_End_0_Seq.gltf" },
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
	m_scabbardModel = std::make_shared<KdModelData>();
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
	if (!kSwordModelPath.empty())		m_swordModel->Load(kSwordModelPath);
	if (!kScabbardModelPath.empty())	m_scabbardModel->Load(kScabbardModelPath);

	// 武器・鞘をどのノードに付けたかを出力ウィンドウに表示（モデルを差し替えた時の確認用）
	for (const auto* pNames : { &kSwordNodeNamesR, &kSwordNodeNamesL, &kScabbardNodeNamesR, &kScabbardNodeNamesL })
	{
		const KdModelWork::Node* pNode = FindFirstNode(*pNames);
		OutputDebugStringA(("武器の取り付け先: " + (pNode ? pNode->m_name : std::string("見つからない")) + "\n").c_str());
	}

	PlayerParamManager::Instance().Load("Asset/Data/PlayerParams.txt");
	PlayerParamManager::Instance().LoadWeaponParams("Asset/Data/WeaponParams.json");

	// アニメーションの読み込み
	LoadAnimations(CreateAnimList());

	// 着地アニメーションで両足が接地するフレームを事前に計算
	for (const auto& animName : kLandingAnimNames)
	{
		if (!HasAnimation(animName)) continue;

		float frame = CalcFootPlantFrame(animName, kFootNodeNames);
		m_landingFrames[animName] = frame;
		OutputDebugStringA(("着地フレーム: " + animName + " = " + std::to_string(frame) + "\n").c_str());
	}

	ChangeState(std::make_shared<PlayerStateIdle>());
	ChangeAnimation("Idle", true);

	// 剣の軌跡（左右）
	m_swordTrailTex = CreateSwordTrailTexture();
	for (SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		pTrail->polygon->SetMaterial(m_swordTrailTex);
		pTrail->polygon->SetLength(kSwordTrailLength * kSwordTrailSubdivision);
	}

	m_pos = { 0, 0.0f, 0.5f };

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

	// アニメーション時間の進行とCalcNodeMatricesの実行はBaseで
	BaseChara::Update();

	ApplyRootMotion(s_attackRootScale, GetAngleRad());

	UpdateInvincibleTimer();

	m_isTrailAddedThisFrame = false;

	if (m_state)
	{
		m_state->Update(this);
	}

	UpdateSwordTrails();
	UpdateAwakening();
	UpdateHpGage();
	UpdateAttackTypeSwitch();

	ApplyRootMotion(m_rootScale, GetAngleRad());

	UpdateWorldMatrix();
}

void Player::PostUpdate()
{
	BaseChara::PostUpdate();
	UpdateWeaponMatrix();
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

	if (m_scabbardModel)
	{
		shader.DrawModel(*m_scabbardModel, m_scabbardWorldR);
		shader.DrawModel(*m_scabbardModel, m_scabbardWorldL);
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
	m_scabbardWorldR = Math::Matrix::Identity;
	m_scabbardWorldL = Math::Matrix::Identity;

	if (!m_model) return;

	// 右
	m_scabbardWorldR = CalcScabbardMatrix(kScabbardNodeNamesR, s_sheathedRotR, s_sheathedPosR);
	m_swordWorldR = CalcSwordMatrix(kSwordNodeNamesR, s_weaponRotR, s_weaponPosR, m_scabbardWorldR);

	// 左
	m_scabbardWorldL = CalcScabbardMatrix(kScabbardNodeNamesL, s_sheathedRotL, s_sheathedPosL);
	m_swordWorldL = CalcSwordMatrix(kSwordNodeNamesL, s_weaponRotL, s_weaponPosL, m_scabbardWorldL);
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

Math::Matrix Player::CalcScabbardMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos) const
{
	const KdModelWork::Node* pNode = FindFirstNode(nodeNames);
	if (!pNode) return Math::Matrix::Identity;

	return CreateAttachMatrix(rot, pos, pNode->m_worldTransform, m_mWorld);
}

Math::Matrix Player::CalcSwordMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos,
	const Math::Matrix& scabbardMat) const
{
	// 納刀時は鞘に合わせる
	if (m_weaponState != WeaponState::Equipped) return scabbardMat;

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

	Math::Vector3 base = GetSwordPoint(swordMat, kSwordBladeBaseY);
	Math::Vector3 tip = GetSwordPoint(swordMat, kSwordBladeTipY);

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

void Player::UpdateAttackTypeSwitch()
{
	// Cキーで攻撃パターンを切り替え（1～kAttackTypeNumをループ）
	if (!m_attackTypeKey.Update()) return;

	m_currentAttackType++;
	if (m_currentAttackType > kAttackTypeNum) m_currentAttackType = 1;

	s_currentAttackType = m_currentAttackType;

	KdDebugGUI::Instance().AddLog
	(
		"[System] 攻撃タイプ切り替え: 0%d\n",
		m_currentAttackType
	);
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
	m_scabbardModel = nullptr;
}

void Player::ChangeState(const std::shared_ptr<PlayerState>& newState)
{
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

std::string Player::GetWeaponAnimName(const std::string& baseName) const
{
	if (m_weaponState == WeaponState::Equipped)
	{
		std::string combatName = kCombatAnimPrefix + baseName;
		if (HasAnimation(combatName)) return combatName;
	}
	return baseName;
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

bool Player::AttackOBB(const Math::Matrix& swordMatrix, bool isCritical)
{
	Math::Matrix obbMatrix = Math::Matrix::CreateTranslation(0.0f, kSwordOBBOffsetY, 0.0f) * swordMatrix;
	Math::Vector3 offset = Math::Vector3::Zero;

	KdCollider::BoxInfo obbInfo(KdCollider::TypeDamage, obbMatrix, offset, kSwordOBBExtents, true);

	// デバッグ用の赤い箱を描画
	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugBox(obbMatrix, kSwordOBBExtents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	bool isHit = false;
	int damage = isCritical ? kCriticalAttackDamage : kAttackDamage;

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

		// 当たった位置にヒットエフェクト（クリティカルは大きく）
		Math::Vector3 hitPos = retList.empty() ? obbMatrix.Translation() : retList.back().m_hitPos;
		EffectPlayer::Play(EffectName::HitEnemy, hitPos, isCritical ? kCriticalHitEffectScale : kHitEffectScale);
	}

	return isHit;
}

bool Player::MoveProcess()
{
	// カメラの向きを基準に移動方向を決める
	Math::Matrix cameraRotY = Math::Matrix::Identity;
	if (auto camera = m_camera.lock())
	{
		cameraRotY = camera->GetRotationYMatrix();
	}

	Math::Vector3 dir = Math::Vector3::TransformNormal(GetInputDir(), cameraRotY);

	UpdateAwakeningInput();

	if (dir.LengthSquared() == 0.0f) return false;

	dir.Normalize();

	// 旋回（1フレームに最大kMaxTurnAngle度まで）
	Math::Matrix nowRot = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 nowDir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), nowRot);
	float angle = DirectX::XMConvertToDegrees(acos(std::clamp(nowDir.Dot(dir), -1.0f, 1.0f)));

	if (angle >= kMinTurnAngle)
	{
		angle = std::min(angle, kMaxTurnAngle);
		if (nowDir.Cross(dir).y >= 0) { m_angle += angle; }
		else { m_angle -= angle; }
	}

	// 座標移動
	float moveSpeed = m_isAwakening ? kAwakeningMoveSpeed : kMoveSpeed;
	m_pos += dir * moveSpeed;
	return true;
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
	return GetSwordPoint(m_swordWorldR, kSwordBladeTipY);
}

Math::Vector3 Player::GetSwordTipPositionL() const
{
	return GetSwordPoint(m_swordWorldL, kSwordBladeTipY);
}

Math::Vector3 Player::GetSwordBasePositionR() const
{
	return GetSwordPoint(m_swordWorldR, kSwordBladeBaseY);
}

Math::Vector3 Player::GetSwordBasePositionL() const
{
	return GetSwordPoint(m_swordWorldL, kSwordBladeBaseY);
}

Math::Vector3 Player::GetCameraTargetPos() const
{
	Math::Vector3 targetPos = m_pos; // XとZは足元の滑らかな座標を使う

	if (m_model)
	{
		// マネキンの腰の骨（pelvis）の高さを使う
		if (const KdModelWork::Node* pNode = m_model->FindNode("pelvis"))
		{
			Math::Matrix m = pNode->m_worldTransform * m_mWorld;
			targetPos.y = m.Translation().y + kCameraTargetOffsetY;
		}
	}
	return targetPos;
}
