#include "Enemy.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"
#include "EnemyState.h"

#include"../../../../Framework/Effekseer/KdEffekseerManager.h"

void Enemy::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_pCollider = std::make_unique<KdCollider>();
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	std::vector<AnimLoadInfo> animList;

	if (m_isBoss)
	{
		// ==========================
		// ボスの設定 (Rampage)
		// ==========================
		m_model->SetModelData("Asset/Models/GameObject/Enemy/Monster2/Base/Rampage.gltf");
		animList = {
			{ "Idle", "Asset/Models/GameObject/Enemy/Monster2/Animation/Idle/Idle/Idle.gltf" },
			{ "Run", "Asset/Models/GameObject/Enemy/Monster2/Animation/Move/Sprint_Biped_Fwd/Sprint_Biped_Fwd.gltf" },
			{ "Attack", "Asset/Models/GameObject/Enemy/Monster2/Animation/Attack/Attack_Melee_A/Attack_Melee_A.gltf" },
			{ "Dead", "Asset/Models/GameObject/Enemy/Monster2/Animation/Hit/Death/Death_A.gltf" },
		};
		m_hp = 150;
		m_scale = 1.75f;
		m_searchRange = 15.0f; // 遠くから気づく
		m_attackRange = 2.5f;  // リーチが長い
	}
	else
	{
		// ==========================
		// ザコ敵の設定
		// ==========================
		m_model->SetModelData("Asset/Models/GameObject/Enemy/Monster2/Base/Rampage.gltf");
		animList = {
			{ "Idle", "Asset/Models/GameObject/Enemy/Monster2/Animation/Idle/Idle/Idle.gltf" },
			{ "Run", "Asset/Models/GameObject/Enemy/Monster2/Animation/Move/Sprint_Biped_Fwd/Sprint_Biped_Fwd.gltf" },
			{ "Attack", "Asset/Models/GameObject/Enemy/Monster2/Animation/Attack/Attack_Melee_A/Attack_Melee_A.gltf" },
			{ "Dead", "Asset/Models/GameObject/Enemy/Monster2/Animation/Hit/Death/Death_A.gltf" },
		};
		m_hp = 50;             
		m_scale = 1.25f;       
		m_searchRange = 8.0f;  
		m_attackRange = 1.2f;  
	}

	LoadAnimations(animList);
	//// Playerを追従させるためにオブジェクトの検索
	//for (auto& obj : SceneManager::Instance().GetObjList())
	//{
	//	auto player = std::dynamic_pointer_cast<Player>(obj);
	//	if (player)
	//	{
	//		SetTarget(player);
	//		break;
	//	}
	//}

	// 3. 初期ステートをIdleに設定
	m_state = std::make_shared<EnemyStateIdle>();
	m_state->ChangeState(this);

	m_pCollider->RegisterCollisionShape
	(
		"Enemy",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);

	m_pos = { 0.0f, 0.0f, 5.0f }; 
	Math::Matrix scale = Math::Matrix::CreateScale(m_scale);
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);
	m_mWorld = scale * trans;

	
}

void Enemy::PostUpdate()
{
	BaseChara::PostUpdate();
}

void Enemy::Update()
{
	BaseChara::Update();
	if (m_invincibleTimer > 0)
	{
		m_invincibleTimer--;
	}

	if (m_state) {
		m_state->Update(this);
	}

	if (m_useRootMotion && m_rootMoveDelta.LengthSquared() > 0.0f)
	{
		Math::Vector3 fixedDelta = m_rootMoveDelta;
		fixedDelta.z *= -1.0f; // Z軸の向き補正

		fixedDelta *= m_rootScale; // 倍率を掛ける

		Math::Matrix rotY = Math::Matrix::CreateRotationY(m_rotY);
		Math::Vector3 move = Math::Vector3::TransformNormal(fixedDelta, rotY);

		m_pos += move; // 実際の座標に足し込む
	}

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto otherEnemy = std::dynamic_pointer_cast<Enemy>(obj);
		// 相手がEnemyで、かつ自分自身ではない場合
		if (otherEnemy && otherEnemy.get() != this)
		{
			Math::Vector3 pushVec = m_pos - otherEnemy->GetPos();
			pushVec.y = 0.0f;

			float dist = pushVec.Length();
			float pushRadius = (m_scale + otherEnemy->m_scale) * 1.0f;

			if (dist > 0.001f && dist < pushRadius)
			{
				pushVec.Normalize();
				// 毎フレームジワジワと押し出す（攻撃中も待機中も反発しあう）
				m_pos += pushVec * (pushRadius - dist) * 0.1f;
			}
		}
	}

	Math::Matrix scale = Math::Matrix::CreateScale(m_scale);
	Math::Matrix rot = Math::Matrix::CreateRotationY(m_rotY);
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);

	m_mWorld = scale * rot * trans;

	if (m_pDebugWire)
	{
		Math::Vector3 hitPos = m_pos + Math::Vector3(0.0f, 1.0f * m_scale, 0.0f);
		m_pDebugWire->AddDebugSphere(hitPos, 1.0f * m_scale, { 0.0f, 1.0f, 0.0f, 1.0f });
	}
}

void Enemy::DrawLit()
{
	//BaseChara::DrawLit();
	//KdShaderManager::Instance().ChangeDepthStencilState(KdDepthStencilState::ZDisable);
	if (m_model)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
	}

}

void Enemy::ChangeState(std::shared_ptr<EnemyState> newState)
{
	m_state = newState;
	if (m_state)
	{
		m_state->ChangeState(this);
	}
}

void Enemy::AttackHit(const Math::Matrix& hitMatrix, const Math::Vector3& extents, bool isCritical)
{
	// 行列（hitMatrix）で既に位置をズラしているので、ローカルオフセットはZeroにする
	Math::Vector3 offset = Math::Vector3::Zero;

	// ① BoxInfoの作成（属性, 行列, オフセット, サイズ, OBBフラグ）
	KdCollider::BoxInfo box(KdCollider::TypeDamage, hitMatrix, offset, extents, true);

	if (m_pDebugWire)
	{
		// ② デバッグボックスの描画（行列, サイズ, オフセット, OBBフラグ, 色）
		m_pDebugWire->AddDebugBox(hitMatrix, extents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 自分自身には当てない
		if (obj.get() == this) continue;

		// 判定処理
		if (obj->Intersects(box, &retList))
		{
			// プレイヤーだった場合、ダメージを与える
			auto player = std::dynamic_pointer_cast<Player>(obj);
			if (player)
			{
				int damage = isCritical ? 10 : 5;
				player->OnDamage(damage, isCritical);
			}
		}
	}
}

Math::Matrix Enemy::GetRightArmMatrix() const
{
	if (m_model)
	{
		const KdModelWork::Node* pNode = m_model->FindNode("lowerarm_l");
		if (pNode)
		{
			return pNode->m_worldTransform * m_mWorld;
		}
	}
	return m_mWorld;
}

void Enemy::Release()
{
	m_model = nullptr;
	m_swordModel = nullptr;
}

void Enemy::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;
	m_invincibleTimer = 30;
	
	// ヒットエフェクトの再生処理
	Math::Vector3 effectPos = m_pos + Math::Vector3(0.0f, 1.0f, 0.0f);

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<EnemyStateDead>());
	}
	else if (isCritical)
	{
		KdDebugGUI::Instance().AddLog("[HIT] 敵にクリティカル！ ダメージ: %d\n", damage);
		ChangeState(std::make_shared<EnemyStateDamage>());
	}
	else
	{
		KdDebugGUI::Instance().AddLog("[HIT] 敵に通常ダメージ！ ダメージ: %d\n", damage);
		ChangeState(std::make_shared<EnemyStateDamage>());
	}

	
}

void Enemy::TurnToPlayer()
{
	auto target = m_wpTarget.lock();
	if (target)
	{
		// プレイヤーへの方向ベクトルを計算
		Math::Vector3 dir = target->GetPos() - m_pos;
		dir.y = 0.0f; // 上下方向は無視する

		if (dir.LengthSquared() > 0.0f)
		{
			dir.Normalize();
			// プレイヤーの方向を向くように角度（m_rotY）を更新する
			m_rotY = atan2(dir.x, dir.z) + 3.141592f;
		}
	}
}