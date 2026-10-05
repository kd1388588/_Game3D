#include "EnemyState.h"
#include "Enemy.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"

void EnemyStateIdle::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Idle", true, false, 5.0f);
	enemy->SetAnimationSpeed(1.0f);

	enemy->SetUseRootMotion(false);
}

void EnemyStateIdle::Update(Enemy* enemy)
{
	// ターゲット（プレイヤー）の情報を取得
	auto spTarget = enemy->GetTarget().lock();
	if (!spTarget) return;

	// プレイヤーとの距離を計算
	Math::Vector3 dir = spTarget->GetPos() - enemy->GetPos();
	dir.y = 0.0f;
	float dist = dir.Length();

	if (dist < enemy->GetSearchRange())
	{
		enemy->ChangeState(std::make_shared<EnemyStateChase>());
	}
}

void EnemyStateChase::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Run", true);
	enemy->SetAnimationSpeed(1.0f);
}

void EnemyStateChase::Update(Enemy* enemy)
{
	auto spTarget = enemy->GetTarget().lock();
	if (!spTarget) return;

	Math::Vector3 dir = spTarget->GetPos() - enemy->GetPos();
	dir.y = 0.0f;
	float dist = dir.Length();

	if (dist > enemy->GetSearchRange() + 2.0f)
	{
		enemy->ChangeState(std::make_shared<EnemyStateIdle>());
		return;
	}

	if (dist <= enemy->GetAttackRange())
	{
		enemy->ChangeState(std::make_shared<EnemyStateAttack>());
		return;
	}

	float speed = 0.1f; // 走るスピード
	dir.Normalize();
	Math::Vector3 nextPos = enemy->GetPos();
	nextPos += dir * speed;
	enemy->SetPos(nextPos);

	enemy->TurnToPlayer(); // プレイヤーの方を向く
}

void EnemyStateAttack::ChangeState(Enemy* enemy)
{
	m_isCritical = false;
	m_hasAttacked = false; 

	enemy->ChangeAnimation("Attack", false, true);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(true);
	enemy->TurnToPlayer();
}

void EnemyStateAttack::Update(Enemy* enemy)
{
	float animTime = enemy->GetAnimTime();

	// 15フレーム目を超えたら1回だけ判定を出す
	if (animTime >= 15.0f && animTime <= 45.0f)
	{
		// 1. 右肘の行列を取得
		Math::Matrix armMat = enemy->GetRightArmMatrix();

		// 2. 右腕はマイナスX方向に伸びているため、X軸のマイナス側へズラす
		// 肘(lowerarm_r)と手首(hand_r)の中間点あたり(-0.25f)を指定
		Math::Matrix offsetMat = Math::Matrix::CreateTranslation(0.25f, 0.0f, 0.0f);
		Math::Matrix hitMat = offsetMat * armMat;

		// 3. ボックスのサイズを設定（Xが腕の長さ、YとZが太さ）
		// 腕の長さ(0.48)をカバーできるように、Xの半径を0.35f（全長0.7）
		Math::Vector3 extents(0.73f, 0.35f, 0.35f);

		enemy->AttackHit(hitMat, extents, m_isCritical);
		m_hasAttacked = true;
	}

	if (enemy->IsAnimEnd())
	{
		enemy->ChangeState(std::make_shared<EnemyStateIdle>());
	}
}

// ==========================================
// ダメージステート
// ==========================================
void EnemyStateDamage::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Damage", false, true);
	enemy->SetAnimationSpeed(1.0f);
}

void EnemyStateDamage::Update(Enemy* enemy)
{
	// アニメーションが最後まで再生されたらIdleに戻る
	if (enemy->IsAnimEnd())
	{
		enemy->ChangeState(std::make_shared<EnemyStateIdle>());
	}
}

// ==========================================
// 死亡ステート
// ==========================================

void EnemyStateDead::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Dead", false, true,5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false); // 死ぬ時はその場に留まる
}

void EnemyStateDead::Update(Enemy* enemy)
{
	if (enemy->IsAnimEnd())
	{
		enemy->SetDead(); // m_isExpired = true となり、キャラが消滅する
	}
}