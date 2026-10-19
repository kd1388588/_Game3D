#include "EnemyState.h"
#include "Enemy.h"
#include "../Player/Player.h"
#include "../../../Scene/SceneManager.h"
#include "../../Effect/EffectPlayer.h"

namespace
{
	constexpr float kChaseSpeed				= 0.1f;		// 走るスピード
	constexpr float kLoseSightMargin		= 2.0f;		// 索敵範囲からこれだけ離れたら追跡をやめる

	// 攻撃判定を出すアニメーションフレームの範囲
	constexpr float kAttackHitStartFrame	= 15.0f;
	constexpr float kAttackHitEndFrame		= 45.0f;

	// 撃破エフェクトの大きさ（敵の体の大きさに掛ける）
	constexpr float kDeathEffectScaleRate	= 0.2f;

	// 飛びかかり攻撃（ボスもザコも。使った後の CT は Enemy.cpp の敵の種類ごとのパラメータ）
	constexpr float kJumpAttackMinDist		= 4.0f;		// プレイヤーがこれより遠く、
	constexpr float kJumpAttackMaxDist		= 14.0f;	// これより近い時に使う
	constexpr int	kJumpAttackChance		= 3;		// 使える距離にいる時、1フレームに使う確率（％）

	// 攻撃
	constexpr float kAttackTurnEndFrame		= 12.0f;	// 攻撃のこのフレームまでは、プレイヤーの方へ向きを変える

	// 攻撃の後のすき（この間は次の攻撃をしてこない。プレイヤーが反撃できる時間）
	constexpr int	kAttackRecoverFrame		= 50;		// ザコ（約0.9秒）
	constexpr int	kBossAttackRecoverFrame	= 35;		// ボス（約0.7秒）
	constexpr int	kAttackRecoverRandom	= 20;		// ばらつき（全員が同じリズムで攻撃してこないように）
	constexpr int	kJumpAttackRecoverFrame	= 50;		// 飛びかかり攻撃の後
	constexpr float	kRecoverTurnSpeed		= 0.05f;	// すきの間に、プレイヤーの方へ向きを変える速さ
	constexpr int	kMaxLeapFrame			= 180;		// 空中にいる時間の上限（地面が無い所へ飛んだ時のため）
	constexpr int	kMaxLandingPredictFrame	= 120;		// 着地予測を行う最大フレーム数
	constexpr float kJumpAttackBlendFrame	= 5.0f;

	// アニメーション（Ability_GroundSmash）を調べた結果
	//   Start（50フレーム）：最初から空中にいて、両手を振り上げながら上がる
	//   Loop（2フレーム）　：一番高い所の姿勢のまま
	//   End（42フレーム）　：6フレーム目で足が地面に着き、10フレーム目で両手を地面に叩きつける。その後起き上がる
	constexpr float kEndLandFrame			= 6.0f;		// End で足が地面に着くフレーム
	constexpr float kEndSmashFrame			= 10.0f;	// End で両手を地面に叩きつけるフレーム（ここで攻撃する）
	constexpr float kWindupPoseTime			= 12.6f;	// 構えに使う姿勢（End の、両手を地面に着いてしゃがんだ所）
	constexpr int	kWindupFrame			= 24;		// 構えている時間（プレイヤーが回避・ガードの準備をする時間）
	constexpr float kWindupBlendFrame		= 8.0f;
	const std::string kJumpAttackStartAnim	= "JumpAttackStart";
	const std::string kJumpAttackLoopAnim	= "JumpAttackLoop";
	const std::string kJumpAttackEndAnim	= "JumpAttackEnd";
}

// ターゲットへの水平方向のベクトルを取得（ターゲットがいなければ false）
bool EnemyState::GetFlatVecToTarget(Enemy* enemy, Math::Vector3& outVec)
{
	auto spTarget = enemy->GetTarget().lock();
	if (!spTarget) return false;

	outVec = spTarget->GetPos() - enemy->GetPos();
	outVec.y = 0.0f;
	return true;
}

// ※敵のアニメーションには、体を前へ進める動き（ルートモーション）を使わない
//   （前の状態のまま残ると、追いかけている途中に変な方向へ進んでしまうため、
//    どの状態でも必ず false にする。移動はプログラムで行う）

void EnemyStateIdle::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Idle", true, false, 5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false);
}

void EnemyStateIdle::Update(Enemy* enemy)
{
	// 攻撃の後のすき：プレイヤーの方をゆっくり向くだけで、動き出さない
	if (m_waitFrame > 0)
	{
		m_waitFrame--;
		enemy->TurnToPlayerSmooth(kRecoverTurnSpeed);
		return;
	}

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
	enemy->ChangeAnimation("Run", true, false, 5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false);
	enemy->TurnToPlayer();
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

	// ボス：少し離れている時は、ときどき飛びかかってくる
	if (dist >= kJumpAttackMinDist && dist <= kJumpAttackMaxDist && enemy->CanJumpAttack()
		&& KdRandom::GetInt(0, 99) < kJumpAttackChance)
	{
		enemy->ChangeState(std::make_shared<EnemyStateJumpAttack>());
		return;
	}

	vecToTarget.Normalize();
	enemy->SetPos(enemy->GetPos() + vecToTarget * kChaseSpeed);

	enemy->TurnToPlayer(); // プレイヤーの方を向く
}

void EnemyStateAttack::ChangeState(Enemy* enemy)
{
	m_isCritical = false;

	enemy->ChangeAnimation("Attack", false, true, 5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false);
	enemy->TurnToPlayer();

	// 攻撃のアニメーション中は、殴られてもひるまない（ダメージだけ受ける）
	// 攻撃していない時（待機・追いかけ中など）は、殴られるとひるむ
	enemy->SetSuperArmor(true);
}

void EnemyStateAttack::Update(Enemy* enemy)
{
	float animTime = enemy->GetAnimTime();

	// 振りかぶっている間は、プレイヤーの方へ向きを変え続ける（振り下ろしてからは変えない）
	// （前は攻撃を始めた瞬間の向きのままだったので、プレイヤーが動くと違う方向を殴っていた）
	if (animTime < kAttackTurnEndFrame)
	{
		enemy->TurnToPlayerSmooth(Enemy::GetAttackTurnSpeed());
	}

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
		// 攻撃が終わったら、少しの間すきを作る（連続で殴ってこないように）
		int recover = enemy->IsBoss() ? kBossAttackRecoverFrame : kAttackRecoverFrame;
		recover += KdRandom::GetInt(0, kAttackRecoverRandom);
		enemy->ChangeState(std::make_shared<EnemyStateIdle>(recover));
	}
}

// ==========================================
// 飛びかかり攻撃（ボス）
// ==========================================
void EnemyStateJumpAttack::ChangeState(Enemy* enemy)
{
	m_phase = Phase::Windup;
	m_timer = 0;
	m_isSmashDone = false;

	enemy->StartJumpAttackCooldown();
	enemy->TurnToPlayer();
	enemy->SetUseRootMotion(false);		// 移動はプログラムで放物線を描いて飛ぶ
	enemy->SetSuperArmor(true);			// 攻撃の途中で殴られても止まらない

	// 構え：着地のアニメーションの「両手を地面に着いてしゃがんだ姿勢」で止めて、力をためる
	// （飛び上がるアニメーションは最初から空中にいるので、構えには使えない）
	// （再生速度を 0 にするとブレンドが進まないので、速度は 1 のまま、毎フレーム同じ時間に戻す）
	enemy->ChangeAnimation(kJumpAttackEndAnim, false, true, kWindupBlendFrame);
	enemy->SetAnimTime(kWindupPoseTime);
	enemy->SetAnimationSpeed(1.0f);
}

void EnemyStateJumpAttack::StartSmashAnim(Enemy* enemy, int framesToLand)
{
	// 着地のアニメーションは kEndLandFrame で足が地面に着くので、
	// 実際に着地する時にそのフレームになるよう、途中から再生する
	float startTime = std::max(kEndLandFrame - static_cast<float>(framesToLand), 0.0f);
	enemy->ChangeAnimation(kJumpAttackEndAnim, false, true, 2.0f);
	enemy->SetAnimTime(startTime);
	enemy->SetAnimationSpeed(1.0f);
	m_phase = Phase::Smash;
}

void EnemyStateJumpAttack::Update(Enemy* enemy)
{
	switch (m_phase)
	{
	case Phase::Windup:
		// 構えている間は、プレイヤーの方を向き続け、しゃがんだ姿勢のまま止めておく
		enemy->TurnToPlayer();
		enemy->SetAnimTime(kWindupPoseTime);

		// 構え終わったら、その時のプレイヤーの位置へ飛び上がる
		if (++m_timer >= kWindupFrame)
		{
			if (auto target = enemy->GetTarget().lock())	enemy->StartLeap(target->GetPos());
			else											enemy->StartLeap(enemy->GetPos());

			// 飛び上がるアニメーション（両手を振り上げる）を、上がりきる時間に合わせて再生する
			enemy->ChangeAnimation(kJumpAttackStartAnim, false, true, kJumpAttackBlendFrame);
			float riseFrames = std::max(enemy->GetLeapRiseFrames(), 1.0f);
			float animLength = enemy->GetAnimLength();
			enemy->SetAnimationSpeed((animLength > 0.0f) ? animLength / riseFrames : 1.0f);

			m_phase = Phase::Rise;
			m_timer = 0;
		}
		break;

	case Phase::Rise:
	case Phase::Hang:
	{
		// 空中：プレイヤーの方へ飛んでいく
		enemy->UpdateLeapMove();
		m_timer++;

		// 上がりきったら、一番高い所の姿勢のまま落ちる
		if (m_phase == Phase::Rise && enemy->IsAnimEnd())
		{
			enemy->ChangeAnimation(kJumpAttackLoopAnim, true, false, 3.0f);
			enemy->SetAnimationSpeed(1.0f);
			m_phase = Phase::Hang;
		}

		// 着地の少し前から、叩きつけるアニメーションを始める（着地の瞬間に足が地面に着くように）
		int framesToLand = enemy->PredictLandingFrames(kMaxLandingPredictFrame);
		if (framesToLand >= 0 && framesToLand <= static_cast<int>(kEndLandFrame))
		{
			StartSmashAnim(enemy, framesToLand);
		}
		else if (enemy->IsLeapLanded() || m_timer >= kMaxLeapFrame)
		{
			// 予測が間に合わなかった（高い所に着地した等）
			StartSmashAnim(enemy, 0);
		}
		break;
	}

	case Phase::Smash:
		// 着地するまでは前へ進み、着地したら止まる
		if (enemy->IsLeapLanded())	enemy->StopLeap();
		else						enemy->UpdateLeapMove();

		// 両手を地面に叩きつけた瞬間に、周りを攻撃する
		if (!m_isSmashDone && enemy->GetAnimTime() >= kEndSmashFrame)
		{
			enemy->StopLeap();
			enemy->GroundSmash();
			m_isSmashDone = true;
		}

		// 叩きつけた後のすき（起き上がるまで動かない。プレイヤーの反撃のチャンス）
		if (enemy->IsAnimEnd())
		{
			if (!m_isSmashDone) enemy->GroundSmash();
			enemy->ChangeState(std::make_shared<EnemyStateIdle>(kJumpAttackRecoverFrame));
		}
		break;
	}
}

// ==========================================
// ダメージステート
// ==========================================
void EnemyStateDamage::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Damage", false, true, 3.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false);
}

void EnemyStateDamage::Update(Enemy* enemy)
{
	if (!enemy->IsAnimEnd()) return;

	// のけぞりを続ける間（プレイヤーのATBコマンドの技を受けている間）は、のけぞりをもう一度再生する
	// （その間は攻撃してこない）
	if (enemy->IsStunned())
	{
		enemy->ChangeAnimation("Damage", false, true, 3.0f);
		return;
	}

	// アニメーションが最後まで再生されたらIdleに戻る
	enemy->ChangeState(std::make_shared<EnemyStateIdle>());
}

// ==========================================
// 死亡ステート
// ==========================================
void EnemyStateDead::ChangeState(Enemy* enemy)
{
	enemy->ChangeAnimation("Dead", false, true, 5.0f);
	enemy->SetAnimationSpeed(1.0f);
	enemy->SetUseRootMotion(false); // 死ぬ時はその場に留まる

	// 撃破エフェクト（体の中心あたりに、体の大きさに合わせて出す）
	float bodyScale = enemy->GetBodyScale();
	Math::Vector3 effectPos = enemy->GetPos() + Math::Vector3(0.0f, bodyScale, 0.0f);
	// ビルボード：エフェクトの板がいつもカメラの方を向くように、カメラの向き（ビュー行列の逆）で回す
	// （そのままだと、横から見た時に板が薄く見えてしまう）
	Math::Matrix camRot = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	camRot.Translation(Math::Vector3::Zero);
	Math::Matrix effectMat = Math::Matrix::CreateScale(bodyScale * kDeathEffectScaleRate)
		* camRot
		* Math::Matrix::CreateTranslation(effectPos);
	EffectPlayer::Play(EffectName::EnemyDeath, effectMat);
}

void EnemyStateDead::Update(Enemy* enemy)
{
	if (enemy->IsAnimEnd())
	{
		enemy->SetDead(); // m_isExpired = true となり、キャラが消滅する
	}
}
