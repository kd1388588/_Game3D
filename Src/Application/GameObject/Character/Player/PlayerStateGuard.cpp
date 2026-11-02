// PlayerStateGuard.cpp
// ガードのステート（ガード・ガードを崩された）
// ※ステートのクラスの宣言は PlayerState.h にまとめてある

#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../Scene/SceneManager.h"

#include "PlayerStateConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerStateConst;

void PlayerStateGuard::ChangeState(Player* player)
{
	UseRootMotion(player);	// 受け止めた時に押し下がる動きも、アニメーションどおりに動かす
	ChangePhase(player, m_phase);
}

void PlayerStateGuard::ChangePhase(Player* player, Phase phase)
{
	m_phase = phase;

	switch (phase)
	{
	case Phase::Start:
		player->ChangeAnimation(kGuardStartAnimName, false, true, kGuardBlendFrame);
		player->SetAnimationSpeed(kGuardStartAnimSpeed);
		break;
	case Phase::Loop:
		player->ChangeAnimation(kGuardLoopAnimName, true, false, kGuardBlendFrame);
		player->SetAnimationSpeed(1.0f);
		break;
	case Phase::Hit:
		// 続けて受けた時も最初から再生する
		player->ChangeAnimation(kGuardHitAnimName, false, true, 2.0f);
		player->SetAnimationSpeed(1.0f);
		break;
	case Phase::End:
		player->ChangeAnimation(kGuardEndAnimName, false, true, kGuardBlendFrame);
		player->SetAnimationSpeed(1.0f);
		break;
	}

	// アニメーションの動きどおりに動かす（ガード歩きの間だけ、UpdateGuardMove で 0 にする）
	player->SetRootMotionScale(1.0f);

	// 構えを解いている時以外はガード中
	player->SetGuarding(phase != Phase::End);
}

void PlayerStateGuard::Update(Player* player)
{
	// 構えを解いている途中
	if (m_phase == Phase::End)
	{
		// もう一度 Shift ／ RB を押したら、すぐ構え直す
		if (player->IsGuardDashDown() && player->CanGuard() && !player->CheckMoveInput())
		{
			ChangePhase(player, Phase::Start);
			return;
		}

		// 少し進んだら、攻撃・回避・移動などに移れる
		float animLength = player->GetAnimLength();
		if (animLength <= 0.0f || player->GetAnimTime() >= animLength * kGuardEndCancelRate)
		{
			if (TryCommonTransition(player)) return;
			if (player->CheckMoveInput())
			{
				player->ChangeState(std::make_shared<PlayerStateRun>());
				return;
			}
		}

		if (player->IsAnimEnd())
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
		return;
	}

	// ガード中の印（Player は毎フレーム消すので、付け直す）
	player->SetGuarding(true);

	// ガード中でも、攻撃・回避・ジャンプ・アビリティはすぐ出せる
	if (TryCommonTransition(player)) return;

	switch (m_phase)
	{
	case Phase::Start:
		// 構え終わったら、構えたままのループへ（Shift ／ RB を離していたら、そのまま構えを解く）
		if (player->IsAnimEnd())
		{
			ChangePhase(player, player->IsGuardDashDown() ? Phase::Loop : Phase::End);
		}
		break;

	case Phase::Loop:
		// Shift ／ RB を離したら構えを解く（移動キーを押していれば、そのまま走り出す）
		if (!player->IsGuardDashDown())
		{
			if (player->CheckMoveInput())	player->ChangeState(std::make_shared<PlayerStateRun>());
			else							ChangePhase(player, Phase::End);
			return;
		}

		// 構えたまま歩く・止まる
		UpdateGuardMove(player);
		break;

	case Phase::Hit:
		// 受け止める動きが終わったら、ガード・ダッシュのキー（Shift ／ RB）を押していれば構えたまま、離していれば構えを解く
		if (player->IsAnimEnd())
		{
			ChangePhase(player, player->IsGuardDashDown() ? Phase::Loop : Phase::End);
		}
		break;

	default:
		break;
	}
}

void PlayerStateGuard::UpdateGuardMove(Player* player)
{
	// カメラの方（敵がいる方）を向いたまま歩く
	player->TurnTowardCameraForward(kGuardTurnAngle);

	Math::Vector3 inputDir;
	if (!player->GetMoveInputDir(inputDir))
	{
		// 止まっている：構えたままのアニメーション（アニメーションの動きどおり）
		player->ChangeAnimation(kGuardLoopAnimName, true, false, kGuardWalkBlendFrame);
		player->SetAnimationSpeed(1.0f);
		player->SetRootMotionScale(1.0f);
		return;
	}

	// 歩く方向に一番近い向きのアニメーションを選び、移動の速さに合わせて再生速度を変える
	float moveSpeed = player->GetGuardMoveSpeed();
	float animSpeed = 1.0f;
	const std::string& animName = player->SelectGuardWalkAnim(inputDir, moveSpeed, animSpeed);
	if (!animName.empty())
	{
		player->ChangeAnimation(animName, true, false, kGuardWalkBlendFrame);
		player->SetAnimationSpeed(animSpeed);
	}

	// 移動は入力した方向へぴったり動かす（アニメーションの動きでは動かさない）
	player->SetRootMotionScale(0.0f);
	player->SetPos(player->GetPos() + inputDir * moveSpeed);
}

void PlayerStateGuardBreak::ChangeState(Player* player)
{
	player->ChangeAnimation(kGuardBreakAnimName, false, true, 2.0f);
	player->SetAnimationSpeed(1.0f);
	UseRootMotion(player);	// よろけて下がる動きは、アニメーションどおりに動かす
}

void PlayerStateGuardBreak::Update(Player* player)
{
	// よろけている間は何もできない
	if (player->IsAnimEnd())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

float PlayerStateGuard::GetFootstepVolume() const
{
	// 構えたまま歩いている時だけ（受け止めた・構え始めなどの足の動きでは鳴らさない）
	return (m_phase == Phase::Loop) ? kGuardFootstepVolume : 0.0f;
}
