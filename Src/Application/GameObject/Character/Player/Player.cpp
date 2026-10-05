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
#include "../../../Scene/GameScene/GameScene.h"

Math::Vector3 Player::s_weaponRotR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponRotL = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_weaponPosL = { 0.0f, 0.0f, 0.0f };

Math::Vector3 Player::s_sheathedRotR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedPosR = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedRotL = { 0.0f, 0.0f, 0.0f };
Math::Vector3 Player::s_sheathedPosL = { 0.0f, 0.0f, 0.0f };

int Player::s_currentAttackType = 1;
float Player::s_evadeSpeed = 0.4f;
float Player::s_dashSpeed = 0.5f;
float Player::s_attackRootScale = 0.2f;
float Player::s_manualStepSpeed = 0.3f;

void Player::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_swordModel = std::make_shared<KdModelData>();
	m_scabbardModel = std::make_shared<KdModelData>();
	m_swordTrail = std::make_shared<KdTrailPolygon>();

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
	std::vector<AnimLoadInfo> animList =
	{
		{ "Idle",			"Asset/Models/GameObject/Player/Kari/Sequence1/01_Idle/01_Idle/AS_Idle_Seq/AS_Idle_Seq.gltf" },
		{ "Equip",			"Asset/Models/GameObject/Player/Kari/Sequence1/01_Idle/01_Idle/AS_Idle_to_Idle_Combat_Seq/AS_Idle_to_Idle_Combat_Seq.gltf" },
		{ "Unequip",		"Asset/Models/GameObject/Player/Kari/Sequence1/01_Idle/02_Idle_Combat/AS_Idle_Combat_to_Idle_Seq/AS_Idle_Combat_to_Idle_Seq.gltf" },
		{ "Combat_Idle",	"Asset/Models/GameObject/Player/Kari/Sequence1/01_Idle/02_Idle_Combat/AS_Idle_Combat_Seq/AS_Idle_Combat_Seq.gltf" },
		{ "Evade",			"Asset/Models/GameObject/Player/Kari/Sequence1/07_Roll/02_Roll_Combat/AS_Roll_Combat_F_0_Seq/AS_Roll_Combat_F_0_Seq.gltf" },
		{ "Dash",			"Asset/Models/GameObject/Player/Kari/Sequence1/06_Dodge/01_Dodge/AS_Dodge_F_0_Seq/AS_Dodge_F_0_Seq.gltf" },
		{ "Combat_Run",		"Asset/Models/GameObject/Player/Kari/Sequence1/04_Run/02_Run_Combat/01_Run_Combat_F_0/AS_Run_Combat_F_0_Loop_Seq/AS_Run_Combat_F_0_Loop_Seq.gltf" },
		{ "Run",			"Asset/Models/GameObject/Player/Kari/Sequence1/04_Run/01_Run/01_Run_F_0/AS_Run_F_0_Loop_Seq/AS_Run_F_0_Loop_Seq.gltf" },
		{ "JumpStart",		"Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/01_Jump/01_Jump_0/AS_Jump_Start_0_Seq/AS_Jump_Start_0_Seq.gltf"},
		{ "JumpLoop",		"Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/01_Jump/01_Jump_0/AS_Jump_Loop_0_Seq/AS_Jump_Loop_0_Seq.gltf"},
		{ "JumpEnd",		"Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/01_Jump/01_Jump_0/AS_Jump_End_0_Seq/AS_Jump_End_0_Seq.gltf"},
		{ "Combat_JumpStart","Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/02_Jump/01_Jump_0/AS_Jump_Start_0_Seq/AS_Jump_Start_0_Seq.gltf"},
		{ "Combat_JumpLoop","Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/02_Jump/01_Jump_0/AS_Jump_Loop_0_Seq/AS_Jump_Loop_0_Seq.gltf"},
		{ "Combat_JumpEnd",	"Asset/Models/GameObject/Player/Kari/Sequence1/05_Jump/02_Jump/01_Jump_0/AS_Jump_End_0_Seq/AS_Jump_End_0_Seq.gltf"},
		{ "Damage",			"Asset/Models/GameObject/Player/Kari/Sequence1/08_Hit/01_Hit/AS_Hit_F_Seq/AS_Hit_F_Seq.gltf" },
		{ "Death",			"Asset/Models/GameObject/Player/Kari/Sequence1/08_Hit/01_Hit/AS_Hit_Death_Seq/AS_Hit_Death_Seq.gltf" },
	};

	for (int type = 1; type <= 5; ++type)
	{
		for (int step = 1; step <= 4; ++step)
		{
			std::string typeStr = "0" + std::to_string(type);
			std::string stepStr = "0" + std::to_string(step);

			if (type == 4 && step == 3)
			{
				std::string basePath = "Asset/Models/GameObject/Player/Kari/Sequence1/02_Attack/04_Combo_Attack_04/AS_Combo_Attack_04_03_";
				animList.push_back({ "Attack_4_3_Start", basePath + "Start_Seq/AS_Combo_Attack_04_03_Start_Seq.gltf" });
				animList.push_back({ "Attack_4_3_Loop",  basePath + "Loop_Seq/AS_Combo_Attack_04_03_Loop_Seq.gltf" });
				animList.push_back({ "Attack_4_3_End",   basePath + "End_Seq/AS_Combo_Attack_04_03_End_Seq.gltf" });
			}
			else
			{
				std::string animName = "Attack_" + std::to_string(type) + "_" + std::to_string(step);
				std::string path = "Asset/Models/GameObject/Player/Kari/Sequence1/02_Attack/"
					+ typeStr + "_Combo_Attack_" + typeStr + "/AS_Combo_Attack_"
					+ typeStr + "_" + stepStr + "_Seq/AS_Combo_Attack_"
					+ typeStr + "_" + stepStr + "_Seq.gltf";

				animList.push_back({ animName, path });
			}
		}
	}

	LoadAnimations(animList);

	ChangeState(std::make_shared<PlayerStateIdle>());
	ChangeAnimation("Idle", true);

	// Effect読込
	m_swordTrail->SetMaterial("Asset/Textures/_GameObject/_Effect/_Sword_Trail.png");	

	m_pos = { 0, 0.0f, 0.5f };

	m_maxHp = 100;
	m_hp = m_maxHp;

	std::shared_ptr<HPGage> hpGage = std::make_shared<HPGage>();
	hpGage->Init();

	m_owner->AddObject(hpGage);
	m_wpHpGage = hpGage;
}

void Player::Update()
{
	if (m_hitStopTimer > 0)
	{
		m_hitStopTimer--;

		// 座標の更新（描画）だけは行って画面に表示し続ける
		Math::Matrix m_scale = Math::Matrix::CreateScale(1);
		Math::Matrix m_rotate = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
		Math::Matrix m_trans = Math::Matrix::CreateTranslation(m_pos);
		m_mWorld = m_scale * m_rotate * m_trans;

		return; // ここで強制終了させることで、アニメーションも移動も止まる！
	}

	// アニメーション時間の進行とCalcNodeMatricesの実行はBaseで
	BaseChara::Update();

	if (m_useRootMotion && m_rootMoveDelta.LengthSquared() > 0.0f)
	{
		Math::Vector3 fixedDelta = m_rootMoveDelta;
		fixedDelta.z *= -1.0f;

		fixedDelta *= s_attackRootScale;

		Math::Matrix rotY = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Vector3 move = Math::Vector3::TransformNormal(fixedDelta, rotY);

		m_pos += move;
	}

	if (m_invincibleTimer > 0)
	{
		m_invincibleTimer--;
	}

	if (m_state)
	{
		m_state->Update(this);
	}

	if (m_isAwakening) {
		m_awakeningTimer--;
		if (m_awakeningTimer <= 0) m_isAwakening = false;
	}

	if (!m_wpHpGage.expired())
	{
		float ratio = (float)m_hp / (float)m_maxHp;
		if (ratio < 0.0f) ratio = 0.0f;

		m_wpHpGage.lock()->SetHpRatio(ratio);
	}

	bool currKeyC = GetAsyncKeyState('C') & 0x8000;
	if (currKeyC && !m_prevKeyC)
	{
		m_currentAttackType++;
		if (m_currentAttackType > 5) m_currentAttackType = 1;

		s_currentAttackType = m_currentAttackType;

		KdDebugGUI::Instance().AddLog
		(
			"[System] 攻撃タイプ切り替え: 0%d\n", 
			m_currentAttackType
		);
	}
	m_prevKeyC = currKeyC;

	if (m_useRootMotion && m_rootMoveDelta.LengthSquared() > 0.0f)
	{
		Math::Vector3 fixedDelta = m_rootMoveDelta;
		fixedDelta.z *= -1.0f;

		fixedDelta *= m_rootScale;

		Math::Matrix rotY = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Vector3 move = Math::Vector3::TransformNormal(fixedDelta, rotY);
		m_pos += move;
	}

	Math::Matrix m_scale = Math::Matrix::CreateScale(1);
	float drawAngle = m_angle + 180.0f;
	Math::Matrix m_rotate = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(drawAngle));
	Math::Matrix m_trans = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = m_scale * m_rotate * m_trans;
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
	if (m_swordTrail)
	{
		// ※バージョンによって m_swordTrail->Draw() の場合と、以下のシェーダー経由の場合があります。
		// エラーが出る場合は m_swordTrail->Draw() を試してください。
		KdShaderManager::Instance().m_StandardShader.DrawPolygon(*m_swordTrail);
	}
}

void Player::DrawLit()
{
	if (m_model)
	{
		KdShaderManager::Instance().ChangeRasterizerState(KdRasterizerState::CullNone);
		KdShaderManager::Instance().ChangeBlendState(KdBlendState::Alpha);
		
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);

		if (m_swordModel)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_swordModel, m_swordWorldR);
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_swordModel, m_swordWorldL);
		}

		if (m_scabbardModel)
		{
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_scabbardModel, m_scabbardWorldR);
			KdShaderManager::Instance().m_StandardShader.DrawModel(*m_scabbardModel, m_scabbardWorldL);
		}

		KdShaderManager::Instance().UndoBlendState();
		KdShaderManager::Instance().UndoRasterizerState();
	}
}


void Player::UpdateWeaponMatrix()
{
	m_swordWorldR = Math::Matrix::Identity;
	m_swordWorldL = Math::Matrix::Identity;
	m_scabbardWorldR = Math::Matrix::Identity;
	m_scabbardWorldL = Math::Matrix::Identity;

	if (m_model)
	{
		// ==========================================
		// --- 右手の鞘 ---
		// ==========================================
		const KdModelWork::Node* pRSpineNode = m_model->FindNode("Weapon_Holder_R");
		if (!pRSpineNode) pRSpineNode = m_model->FindNode("spine_05"); // 見つからない場合の保険

		if (pRSpineNode)
		{
			// GUIの数値を回転と座標に変換して掛け合わせる
			Math::Matrix rot =
				Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(s_sheathedRotR.x))
				* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(s_sheathedRotR.y))
				* Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(s_sheathedRotR.z));
			Math::Matrix trans = Math::Matrix::CreateTranslation(s_sheathedPosR.x, s_sheathedPosR.y, s_sheathedPosR.z);

			m_scabbardWorldR = rot * trans * pRSpineNode->m_worldTransform * m_mWorld;
		}

		// ==========================================
		// --- 右手の剣（装備時） ---
		// ==========================================
		if (m_weaponState == WeaponState::Equipped)
		{
			const KdModelWork::Node* pNodeR = m_model->FindNode("Weapon_R");
			if (pNodeR)
			{
				// GUIの数値を回転と座標に変換して掛け合わせる
				Math::Matrix rot =
					Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(s_weaponRotR.x))
					* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(s_weaponRotR.y))
					* Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(s_weaponRotR.z));
				Math::Matrix trans = Math::Matrix::CreateTranslation(s_weaponPosR.x, s_weaponPosR.y, s_weaponPosR.z);

				m_swordWorldR = rot * trans * pNodeR->m_worldTransform * m_mWorld;
			}
		}
		else
		{
			// 納刀時は鞘に合わせる
			m_swordWorldR = m_scabbardWorldR;
		}

		//// ==========================================
		//// --- 左手の鞘 ---
		//// ==========================================
		const KdModelWork::Node* pLSpineNode = m_model->FindNode("Weapon_Holder_L");
		if (!pLSpineNode) pLSpineNode = m_model->FindNode("spine_05"); // 見つからない場合の保険

		if (pLSpineNode)
		{
			// GUIの数値を回転と座標に変換して掛け合わせる
			Math::Matrix rot =
				Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(s_sheathedRotL.x))
				* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(s_sheathedRotL.y))
				* Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(s_sheathedRotL.z));
			Math::Matrix trans = Math::Matrix::CreateTranslation(s_sheathedPosL.x, s_sheathedPosL.y, s_sheathedPosL.z);

			m_scabbardWorldL = rot * trans * pLSpineNode->m_worldTransform * m_mWorld;
		}

		// ==========================================
		// --- 左手の剣（装備時） ---
		// ==========================================
		if (m_weaponState == WeaponState::Equipped)
		{
			const KdModelWork::Node* pNodeL = m_model->FindNode("Weapon_L");
			if (pNodeL)
			{
				// GUIの数値を回転と座標に変換して掛け合わせる
				Math::Matrix rot =
					Math::Matrix::CreateRotationX(DirectX::XMConvertToRadians(s_weaponRotL.x))
					* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(s_weaponRotL.y))
					* Math::Matrix::CreateRotationZ(DirectX::XMConvertToRadians(s_weaponRotL.z));
				Math::Matrix trans = Math::Matrix::CreateTranslation(s_weaponPosL.x, s_weaponPosL.y, s_weaponPosL.z);

				m_swordWorldL = rot * trans * pNodeL->m_worldTransform * m_mWorld;
			}
		}
		else
		{
			// 納刀時は鞘に合わせる
			m_swordWorldL = m_scabbardWorldL;
		}
	}
}
void Player::Release()
{
	m_model = nullptr;
	m_swordModel = nullptr;
	m_scabbardModel = nullptr;
}

void Player::ChangeState(std::shared_ptr<PlayerState> newState)
{
	m_state = newState;
	if (m_state)
	{
		m_state->ChangeState(this);
	}
}

bool Player::CheckMoveInput()
{
	Math::Vector3 dir = Math::Vector3::Zero;

	if (GetAsyncKeyState('W') & 0x8000)
	{
		dir.z += 1.0f;
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		dir.z -= 1.0f;
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		dir.x -= 1.0f;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		dir.x += 1.0f;
	}

	if (dir.LengthSquared() > 0.0f)
	{
		return true;
	}
	return false;
}

bool Player::CheckAttackInput()
{
	bool currClick = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
	bool trigger = currClick && !m_prevClick;
	m_prevClick = currClick;
	return trigger;
}

bool Player::CheckJumpInput()
{
	bool currSpace = GetAsyncKeyState(VK_SPACE) & 0x8000;
	bool trigger = currSpace && !m_prevSpace; // 押した瞬間だけ true になる
	m_prevSpace = currSpace;
	return trigger;
}

void Player::ChangeAnimationLazy(const std::string& animName, bool isLoop, bool forceRestart, float blendFrame)	
{
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
	}

	// 登録済みの通常のアニメーション変更を呼び出す
	ChangeAnimation(animName, isLoop, forceRestart, blendFrame);
}

bool Player::AttackOBB(const Math::Matrix& swordMatrix, bool isCritical)
{
	// マネキン用の剣に合わせて、Y軸（上方向）へずらす
	Math::Matrix localOffset = Math::Matrix::CreateTranslation(0.0f, 0.6f, 0.0f);
	Math::Matrix obbMatrix = localOffset * swordMatrix;
	Math::Vector3 offset = Math::Vector3::Zero;

	// Y軸方向に長く、XとZを少し太めにした元の設定に戻す
	Math::Vector3 extents(0.1f, 0.55f, 0.1f);

	KdCollider::BoxInfo obbInfo(KdCollider::TypeDamage, obbMatrix, offset, extents, true);

	// デバッグ用の赤い箱を描画
	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugBox(obbMatrix, extents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	bool isHit = false;

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() == this) continue;

		if (obj->Intersects(obbInfo, &retList))
		{
			auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
			if (enemy)
			{
				int damage = isCritical ? 20 : 10;
				enemy->OnDamage(damage, isCritical);

				isHit = true;
			}
		}
	}

	return isHit;
}

bool Player::CheckEquipInput()
{
	bool currKeyE = GetAsyncKeyState('E') & 0x8000;
	bool trigger = currKeyE && !m_EKey; // 押した瞬間だけ true になる
	m_EKey = currKeyE;
	return trigger;
}

bool Player::MoveProcess()
{
	Math::Vector3 dir = Math::Vector3::Zero;
	bool isMove = false;

	Math::Matrix camera_rect = Math::Matrix::Identity;
	if (!m_camera.expired())
	{
		camera_rect = m_camera.lock()->GetRotationYMatrix();
	}

	if (GetAsyncKeyState('W') & 0x8000)
	{
		dir += Math::Vector3::TransformNormal({ 0, 0, 1 }, camera_rect); isMove = true;
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		dir += Math::Vector3::TransformNormal({ -1, 0, 0 }, camera_rect); isMove = true;
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		dir += Math::Vector3::TransformNormal({ 0, 0, -1 }, camera_rect); isMove = true;
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		dir += Math::Vector3::TransformNormal({ 1, 0, 0 }, camera_rect); isMove = true;
	}

	if (isMove && dir.LengthSquared() == 0.0f)
	{
		isMove = false;
	}
	if (GetAsyncKeyState('U') & 0x8000) 
	{
		m_awakeningGage = m_maxAwakeningGage;
	}

	// Gキーで覚醒発動！
	if (GetAsyncKeyState('G') & 0x8000) 
	{
		if (m_awakeningGage >= m_maxAwakeningGage && !m_isAwakening) 
		{
			m_isAwakening = true;
			m_awakeningGage = 0.0f;
			m_awakeningTimer = 60 * 15;    // 15秒間持続
			m_hp += (int)(m_maxHp * 0.3f); // HP3割回復
			if (m_hp > m_maxHp) m_hp = m_maxHp;
		}
	}

	// 旋回と座標移動
	if (isMove && dir.LengthSquared() != 0.0f)
	{
		dir.Normalize();
		Math::Matrix now_rect = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle));
		Math::Vector3 now_dir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), now_rect);
		float dot = now_dir.Dot(dir);
		dot = acos(std::clamp(dot, -1.0f, 1.0f));
		float angle = DirectX::XMConvertToDegrees(dot);

		if (angle >= 0.1f)
		{
			if (angle > 10.0f) angle = 10.0f;
			Math::Vector3 cross = now_dir.Cross(dir);
			if (cross.y >= 0) { m_angle += angle; }
			else { m_angle -= angle; }
		}

		float moveSpeed = m_isAwakening ? 0.30f : 0.15f; // 覚醒中は2倍のスピード！
		m_pos += dir * moveSpeed;
		return true;
	}
	return false;
}

void Player::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;

	m_invincibleTimer = 30;

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<PlayerStateDead>());
	}
	else if (isCritical || damage >= 20)
	{
		ChangeState(std::make_shared<PlayerStateDamage>());
	}
}

Math::Vector3 Player::GetSwordTipPositionR() const
{
	Math::Vector4 localTip = Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Math::Vector4 worldTip = Math::Vector4::Transform(localTip, m_swordWorldR);
	return Math::Vector3(worldTip.x, worldTip.y, worldTip.z);
}

Math::Vector3 Player::GetSwordTipPositionL() const
{
	Math::Vector4 localTip = Math::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Math::Vector4 worldTip = Math::Vector4::Transform(localTip, m_swordWorldL);
	return Math::Vector3(worldTip.x, worldTip.y, worldTip.z);
}

Math::Vector3 Player::GetSwordBasePositionR() const
{

	Math::Vector4 localBase = Math::Vector4(0.0f, 0.0f, 0.2f, 1.0f);
	Math::Vector4 worldBase = Math::Vector4::Transform(localBase, m_swordWorldR);
	return Math::Vector3(worldBase.x, worldBase.y, worldBase.z);
}

Math::Vector3 Player::GetSwordBasePositionL() const
{
	Math::Vector4 localBase = Math::Vector4(0.0f, 0.0f, 0.2f, 1.0f);
	Math::Vector4 worldBase = Math::Vector4::Transform(localBase, m_swordWorldL);
	return Math::Vector3(worldBase.x, worldBase.y, worldBase.z);
}

Math::Vector3 Player::GetCameraTargetPos() const
{
	Math::Vector3 targetPos = m_pos; // XとZは足元の滑らかな座標を使う

	if (m_model)
	{
		// マネキンの腰の骨（pelvis）を探す
		const KdModelWork::Node* pNode = m_model->FindNode("pelvis");
		if (pNode)
		{
			Math::Matrix m = pNode->m_worldTransform * m_mWorld;
			targetPos.y = m.Translation().y - 0.9f;
		}
	}
	return targetPos;
}