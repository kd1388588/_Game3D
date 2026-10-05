#include "Enemy.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"
#include "EnemyState.h"

namespace
{
	// 敵の種類ごとのパラメータ
	struct EnemyParam
	{
		int		hp;
		float	scale;
		float	searchRange;	// プレイヤーに気づく距離
		float	attackRange;	// 攻撃を開始する距離
	};

	constexpr EnemyParam kBossParam		= { 150, 1.75f, 15.0f, 2.5f };	// ボス：遠くから気づき、リーチが長い
	constexpr EnemyParam kNormalParam	= { 50,  1.25f, 8.0f,  1.2f };	// ザコ敵

	constexpr int	kDamageInvincibleFrame	= 30;		// 被弾後の無敵時間
	constexpr int	kAttackDamage			= 5;
	constexpr int	kCriticalAttackDamage	= 10;
	constexpr float	kPushStrength			= 0.1f;		// 敵同士の押し出しの強さ（毎フレームの割合）

	const std::string kModelDir = "Asset/Models/GameObject/Enemy/Monster2/";
}

void Enemy::Init()
{
	m_model = std::make_shared<KdModelWork>();
	m_pCollider = std::make_unique<KdCollider>();
	m_pDebugWire = std::make_unique<KdDebugWireFrame>();

	// モデル・アニメーション（現在はボスもザコも同じものを使用）
	m_model->SetModelData(kModelDir + "Base/Rampage.gltf");
	LoadAnimations
	({
		{ "Idle",	kModelDir + "Animation/Idle/Idle/Idle.gltf" },
		{ "Run",	kModelDir + "Animation/Move/Sprint_Biped_Fwd/Sprint_Biped_Fwd.gltf" },
		{ "Attack",	kModelDir + "Animation/Attack/Attack_Melee_A/Attack_Melee_A.gltf" },
		{ "Dead",	kModelDir + "Animation/Hit/Death/Death_A.gltf" },
	});

	// 種類ごとのパラメータ
	const EnemyParam& param = m_isBoss ? kBossParam : kNormalParam;
	SetHp(param.hp);
	m_scale = param.scale;
	m_searchRange = param.searchRange;
	m_attackRange = param.attackRange;

	// 初期ステートをIdleに設定
	ChangeState(std::make_shared<EnemyStateIdle>());

	m_pCollider->RegisterCollisionShape
	(
		"Enemy",
		{ 0.0f, 0.5f, 0.0f },
		0.6f,
		KdCollider::Type::TypeDamage
	);

	m_pos = { 0.0f, 0.0f, 5.0f };
	UpdateWorldMatrix();
}

void Enemy::PostUpdate()
{
	BaseChara::PostUpdate();
}

void Enemy::Update()
{
	BaseChara::Update();

	UpdateInvincibleTimer();

	if (m_state)
	{
		m_state->Update(this);
	}

	ApplyRootMotion(m_rootScale, m_rotY);

	PushAwayFromOtherEnemies();

	UpdateWorldMatrix();

	if (m_pDebugWire)
	{
		Math::Vector3 hitPos = m_pos + Math::Vector3(0.0f, m_scale, 0.0f);
		m_pDebugWire->AddDebugSphere(hitPos, m_scale, { 0.0f, 1.0f, 0.0f, 1.0f });
	}
}

void Enemy::DrawLit()
{
	if (m_model)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
	}
}

void Enemy::UpdateWorldMatrix()
{
	Math::Matrix scale = Math::Matrix::CreateScale(m_scale);
	Math::Matrix rot = Math::Matrix::CreateRotationY(m_rotY);
	Math::Matrix trans = Math::Matrix::CreateTranslation(m_pos);

	m_mWorld = scale * rot * trans;
}

void Enemy::PushAwayFromOtherEnemies()
{
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 相手がEnemyで、かつ自分自身ではない場合
		auto otherEnemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!otherEnemy || otherEnemy.get() == this) continue;

		Math::Vector3 pushVec = m_pos - otherEnemy->GetPos();
		pushVec.y = 0.0f;

		float dist = pushVec.Length();
		float pushRadius = m_scale + otherEnemy->m_scale;

		if (dist > 0.001f && dist < pushRadius)
		{
			pushVec.Normalize();
			// 毎フレームジワジワと押し出す（攻撃中も待機中も反発しあう）
			m_pos += pushVec * (pushRadius - dist) * kPushStrength;
		}
	}
}

void Enemy::ChangeState(const std::shared_ptr<EnemyState>& newState)
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

	KdCollider::BoxInfo box(KdCollider::TypeDamage, hitMatrix, offset, extents, true);

	if (m_pDebugWire)
	{
		m_pDebugWire->AddDebugBox(hitMatrix, extents, offset, true, { 1.0f, 0.0f, 0.0f, 1.0f });
	}

	int damage = isCritical ? kCriticalAttackDamage : kAttackDamage;

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		// 自分自身には当てない
		if (obj.get() == this) continue;
		if (!obj->Intersects(box, &retList)) continue;

		// プレイヤーだった場合、ダメージを与える
		if (auto player = std::dynamic_pointer_cast<Player>(obj))
		{
			player->OnDamage(damage, isCritical);
		}
	}
}

Math::Matrix Enemy::GetRightArmMatrix() const
{
	if (m_model)
	{
		// ※モデルの骨の向きの都合で "lowerarm_l" を使用している
		if (const KdModelWork::Node* pNode = m_model->FindNode("lowerarm_l"))
		{
			return pNode->m_worldTransform * m_mWorld;
		}
	}
	return m_mWorld;
}

void Enemy::Release()
{
	m_model = nullptr;
}

void Enemy::OnDamage(int damage, bool isCritical)
{
	if (m_invincibleTimer > 0) return;
	if (m_hp <= 0) return;

	m_hp -= damage;
	m_invincibleTimer = kDamageInvincibleFrame;

	if (m_hp <= 0)
	{
		ChangeState(std::make_shared<EnemyStateDead>());
		return;
	}

	KdDebugGUI::Instance().AddLog
	(
		isCritical ? "[HIT] 敵にクリティカル！ ダメージ: %d\n" : "[HIT] 敵に通常ダメージ！ ダメージ: %d\n",
		damage
	);
	ChangeState(std::make_shared<EnemyStateDamage>());
}

void Enemy::TurnToPlayer()
{
	auto target = m_wpTarget.lock();
	if (!target) return;

	// プレイヤーへの方向ベクトルを計算（上下方向は無視する）
	Math::Vector3 dir = target->GetPos() - m_pos;
	dir.y = 0.0f;

	if (dir.LengthSquared() > 0.0f)
	{
		dir.Normalize();
		// モデルが逆向きなので180度回す
		m_rotY = atan2(dir.x, dir.z) + DirectX::XM_PI;
	}
}
