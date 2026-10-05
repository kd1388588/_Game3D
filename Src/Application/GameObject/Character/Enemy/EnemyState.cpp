#include "EnemyState.h"
#include "Enemy.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"

namespace
{
	constexpr float kChaseSpeed				= 0.1f;		// 走るスピード
	constexpr float kLoseSightMargin		= 2.0f;		// 索敵範囲からこれだけ離れたら追跡をやめる

	// 攻撃判定を出すアニメーションフレームの範囲
	constexpr float kAttackHitStartFrame	= 15.0f;
	constexpr float kAttackHitEndFrame		= 45.0f;

	// ターゲットへの水平方向のベクトルを取得（ターゲットがいなければ false）
	bool GetFlatVecToTarget(Enemy* enemy, Math::Vector3& outVec)
	{
		auto spTarget = enemy->GetTarget().lock();
		if (!spTarget) return false;

		outVec = spTarget->GetPos() - enemy->GetPos();
		outVec.y = 0.0f;
		return true;
	}
}

void EnemyStateIdle::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Idle", true, false, 5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false);
}

void EnemyStateIdle::Update(Enemy* enemy)
{
	Math::Vector3 vecToTarget;
	if (!GetFlatVecToTarget(enemy, vecToTarget)) return;

	// プレイヤーが索敵範囲に入ったら追跡開始
	if (vecToTarget.Length() < enemy->GetSearchRange())
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
	Math::Vector3 vecToTarget;
	if (!GetFlatVecToTarget(enemy, vecToTarget)) return;

	float dist = vecToTarget.Length();

	if (dist > enemy->GetSearchRange() + kLoseSightMargin)
	{
		enemy->ChangeState(std::make_shared<EnemyStateIdle>());
		return;
	}

	if (dist <= enemy->GetAttackRange())
	{
		enemy->ChangeState(std::make_shared<EnemyStateAttack>());
		return;
	}

	vecToTarget.Normalize();
	enemy->SetPos(enemy->GetPos() + vecToTarget * kChaseSpeed);

	enemy->TurnToPlayer(); // プレイヤーの方を向く
}

void EnemyStateAttack::ChangeState(Enemy* enemy)
{
	m_isCritical = false;

	enemy->ChangeAnimation("Attack", false, true);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(true);
	enemy->TurnToPlayer();
}

void EnemyStateAttack::Update(Enemy* enemy)
{
	float animTime = enemy->GetAnimTime();

	if (animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame)
	{
		// 1. 腕（肘）の行列を取得
		Math::Matrix armMat = enemy->GetRightArmMatrix();

		// 2. 肘と手首の中間点あたりへズラす
		Math::Matrix offsetMat = Math::Matrix::CreateTranslation(0.25f, 0.0f, 0.0f);
		Math::Matrix hitMat = offsetMat * armMat;

		// 3. ボックスのサイズを設定（Xが腕の長さ、YとZが太さ）
		Math::Vector3 extents(0.73f, 0.35f, 0.35f);

		enemy->AttackHit(hitMat, extents, m_isCritical);
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
	enemy->ChangeAnimation("Dead", false, true, 5.0f);
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
