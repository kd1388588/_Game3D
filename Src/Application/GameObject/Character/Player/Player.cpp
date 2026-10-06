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
	constexpr UINT	kSwordTrailLength		= 20;		// 軌跡を残すフレーム数
	const std::string kSwordTrailTexture	= "Asset/Textures/_GameObject/_Effect/_Sword_Trail.png";

	// カメラの注視点を腰の骨から下げる量
	constexpr float kCameraTargetOffsetY	= -0.9f;

	const std::string kAnimDir = "Asset/Models/GameObject/Player/Kari/Sequence1/";

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
		{ "JumpStart",			kAnimDir + "05_Jump/01_Jump/01_Jump_0/AS_Jump_Start_0_Seq/AS_Jump_Start_0_Seq.gltf" },
		{ "JumpLoop",			kAnimDir + "05_Jump/01_Jump/01_Jump_0/AS_Jump_Loop_0_Seq/AS_Jump_Loop_0_Seq.gltf" },
		{ "JumpEnd",			kAnimDir + "05_Jump/01_Jump/01_Jump_0/AS_Jump_End_0_Seq/AS_Jump_End_0_Seq.gltf" },
		{ "Combat_JumpStart",	kAnimDir + "05_Jump/02_Jump_Combat/01_Jump_Combat_0/AS_Jump_Combat_Start_0_Seq/AS_Jump_Combat_Start_0_Seq.gltf" },
		{ "Combat_JumpLoop",	kAnimDir + "05_Jump/02_Jump_Combat/01_Jump_Combat_0/AS_Jump_Combat_Loop_0_Seq/AS_Jump_Combat_Loop_0_Seq.gltf" },
		{ "Combat_JumpEnd",		kAnimDir + "05_Jump/02_Jump_Combat/01_Jump_Combat_0/AS_Jump_Combat_End_0_Seq/AS_Jump_Combat_End_0_Seq.gltf" },
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
Math::Matrix Player::CreateTrailMatrix(const Math::Matrix& swordMat)
{
	Math::Vector3 base = GetSwordPoint(swordMat, kSwordBladeBaseY);
	Math::Vector3 tip = GetSwordPoint(swordMat, kSwordBladeTipY);

	// 帯の幅は「X軸の長さ × 0.5」になるので、刃の長さの2倍を設定する
	Math::Matrix trailMat = Math::Matrix::Identity;
	trailMat.Right((tip - base) * (kSwordTrailWidthRate * 2.0f));
	trailMat.Translation((base + tip) * 0.5f);
	return trailMat;
}

void Player::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_swordModel = std::make_shared<KdModelData>();
	m_scabbardModel = std::make_shared<KdModelData>();
	m_swordTrailR = std::make_shared<KdTrailPolygon>();
	m_swordTrailL = std::make_shared<KdTrailPolygon>();

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
	m_model->SetModelData("Asset/Models/GameObject/Player/Animation/CharaBase/Kari/SK_Mannequin.gltf");
	m_swordModel->Load("Asset/Models/GameObject/Player/Sword/Kari/sword.gltf");
	m_scabbardModel->Load("");

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
	for (const auto& trail : { m_swordTrailR, m_swordTrailL })
	{
		trail->SetMaterial(kSwordTrailTexture);
		trail->SetLength(kSwordTrailLength);
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

void Player::DrawUnLit()
{
	auto& shaderManager = KdShaderManager::Instance();

	// 軌跡の帯は裏からも見えるようにカリングを切る
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Alpha);

	for (const auto& trail : { m_swordTrailR, m_swordTrailL })
	{
		if (trail)
		{
			shaderManager.m_StandardShader.DrawPolygon(*trail);
		}
	}

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
	m_scabbardWorldR = CalcScabbardMatrix("Weapon_Holder_R", s_sheathedRotR, s_sheathedPosR);
	m_swordWorldR = CalcSwordMatrix("Weapon_R", s_weaponRotR, s_weaponPosR, m_scabbardWorldR);

	// 左
	m_scabbardWorldL = CalcScabbardMatrix("Weapon_Holder_L", s_sheathedRotL, s_sheathedPosL);
	m_swordWorldL = CalcSwordMatrix("Weapon_L", s_weaponRotL, s_weaponPosL, m_scabbardWorldL);
}

Math::Matrix Player::CalcScabbardMatrix(const std::string& holderName, const Math::Vector3& rot, const Math::Vector3& pos) const
{
	// ホルダーの骨が見つからない場合は背骨で代用
	const KdModelWork::Node* pNode = m_model->FindNode(holderName);
	if (!pNode) pNode = m_model->FindNode("spine_05");
	if (!pNode) return Math::Matrix::Identity;

	return CreateAttachMatrix(rot, pos, pNode->m_worldTransform, m_mWorld);
}

Math::Matrix Player::CalcSwordMatrix(const std::string& handName, const Math::Vector3& rot, const Math::Vector3& pos,
	const Math::Matrix& scabbardMat) const
{
	// 納刀時は鞘に合わせる
	if (m_weaponState != WeaponState::Equipped) return scabbardMat;

	const KdModelWork::Node* pNode = m_model->FindNode(handName);
	if (!pNode) return Math::Matrix::Identity;

	return CreateAttachMatrix(rot, pos, pNode->m_worldTransform, m_mWorld);
}

void Player::AddSwordTrailPoints()
{
	// 新しい振りの始まりなら、前の振りの軌跡とつながらないように消しておく
	if (!m_isTrailActive)
	{
		m_swordTrailR->ClearPoints();
		m_swordTrailL->ClearPoints();
	}

	m_swordTrailR->AddPoint(CreateTrailMatrix(m_swordWorldR));
	m_swordTrailL->AddPoint(CreateTrailMatrix(m_swordWorldL));

	m_isTrailActive = true;
	m_isTrailAddedThisFrame = true;
}

void Player::UpdateSwordTrails()
{
	if (m_isTrailAddedThisFrame) return;

	// 攻撃していないフレームは、古いポイントから消して軌跡を徐々に短くする
	m_isTrailActive = false;
	for (const auto& trail : { m_swordTrailR, m_swordTrailL })
	{
		if (trail && trail->GetNumPoints() > 0)
		{
			trail->DelPointBack();
		}
	}
}

void Player::UpdateAwakening()
{
	if (!m_isAwakening) return;

	m_awakeningTimer--;
	if (m_awakeningTimer <= 0) m_isAwakening = false;
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

		if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj))
		{
			enemy->OnDamage(damage, isCritical);
			isHit = true;
		}
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
