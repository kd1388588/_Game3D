// PlayerState.cpp
// ステートの共通処理・待機・被弾・死亡
// ※ステートのクラスの宣言は PlayerState.h にまとめてある

#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../Scene/SceneManager.h"

#include "PlayerStateConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerStateConst;

// 前方へ減速しながら移動する（回避用）
void PlayerState::MoveForwardWithDecay(Player* player, float baseSpeed, float moveFrame, bool isFlat)
{
	float animTime = player->GetAnimTime();
	if (animTime >= moveFrame) return;

	float speed = baseSpeed * (1.0f - (animTime / moveFrame));

	Math::Vector3 forward = player->GetMatrix().Backward() * -1.0f;
	if (isFlat)
	{
		forward.y = 0.0f;
		forward.Normalize();
	}

	player->SetPos(player->GetPos() + forward * speed);
	player->BumpHit();
}

void PlayerState::UseRootMotion(Player* player)
{
	player->SetUseRootMotion(true);
	player->SetRootMotionScale(1.0f);
}

void PlayerState::StartEvade(Player* player)
{
	// パニッシャーの時は、オペレーターに戻ってから回避する（FF7リメイクのクラウドと同じ動き）
	if (kEvadeReturnsToOperator && player->IsPunisherMode())
	{
		player->SetBattleMode(BattleMode::Operator);
	}
	player->GainAtbByEvade();	// 回避すると ATB ゲージがたまる
	player->ChangeState(std::make_shared<PlayerStateEvade>());
}

// 待機・移動中に共通の入力によるステート遷移
// 遷移した場合は true を返す
bool PlayerState::TryCommonTransition(Player* player)
{
	// 右クリック：回避
	if (player->CheckEvadeInput())
	{
		StartEvade(player);
		return true;
	}

	// 攻撃（剣は常に抜いているので、いつでも攻撃できる）
	if (player->CheckAttackInput())
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(player->GetCurrentAttackType(), 1));
		return true;
	}

	return false;
}

void PlayerStateIdle::ChangeState(Player* player)
{
	player->ChangeAnimation(kIdleAnimName, true, false, kRunBlendFrame);
	player->SetAnimationSpeed(1.0f);
	UseRootMotion(player);
}

void PlayerStateIdle::Update(Player* player)
{
	if (TryCommonTransition(player)) return;

	if (player->CheckMoveInput())
	{
		// 移動（ガード・ダッシュのキー（Ctrl ／ RB）を押していればダッシュ）
		player->ChangeState(std::make_shared<PlayerStateRun>());
	}
	else if (player->IsGuardDashDown() && player->CanGuard())
	{
		// 止まっている時に Ctrl ／ RB：ガード（構えた後は、移動キーでガードしたまま歩ける）
		player->ChangeState(std::make_shared<PlayerStateGuard>());
	}
}

void PlayerStateDamage::ChangeState(Player* player)
{
	// 仰け反るようなダメージモーションを再生
	player->ChangeAnimation("Damage", false, true);
	UseRootMotion(player);
	player->SetAnimationSpeed(1.0f);
}

void PlayerStateDamage::Update(Player* player)
{
	// 少しの間、後ろへノックバック（アニメーションに後ろへ下がる動きが無い場合だけ）
	if (player->GetAnimTime() < kDamageKnockbackFrame && !player->CurrentAnimHasHorizontalMotion())
	{
		Math::Vector3 backward = player->GetMatrix().Backward();
		backward.y = 0.0f;
		backward.Normalize();

		player->SetPos(player->GetPos() + backward * kDamageKnockbackSpeed);
	}

	if (player->IsAnimEnd())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateDead::ChangeState(Player* player)
{
	player->ChangeAnimation("Death", false, true);
	UseRootMotion(player);	// 倒れる動きはアニメーションどおりに動かす
	player->SetAnimationSpeed(1.0f);
}

void PlayerStateDead::Update(Player* player)
{
	// 死亡モーションの後はゲームオーバー画面へ（GameScene側で判定）
	// 倒れた後に動かないよう、ここでは何もしない
}
