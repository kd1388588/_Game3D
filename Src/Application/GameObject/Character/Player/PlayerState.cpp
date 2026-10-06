#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/Effect.h"
#include "../../../Scene/SceneManager.h"

using InputHelper::IsKeyDown;

namespace
{
	// 攻撃判定を出すアニメーションフレームの範囲
	constexpr float kAttackHitStartFrame	= 10.0f;
	constexpr float kAttackHitEndFrame		= 45.0f;
	constexpr int	kChargeLoopFrame		= 90;		// 4-3 Loop の持続フレーム
	constexpr int	kMaxComboStep			= 5;

	// 回避・ダッシュ・被弾で移動するフレーム数
	constexpr float kEvadeMoveFrame			= 25.0f;
	constexpr float kDamageKnockbackFrame	= 15.0f;
	constexpr float kDamageKnockbackSpeed	= 0.06f;

	// ジャンプ
	constexpr float kJumpPower				= 0.25f;
	constexpr float kFallingGravity			= 0.011f;	// これを超えたら落下中とみなす
	constexpr int	kMaxLandingPredictFrame	= 120;		// 着地予測を行う最大フレーム数
	constexpr float kLandingBlendFrame		= 3.0f;		// 空中ループ→着地アニメのブレンドフレーム数
}

// 前方へ減速しながら移動する（回避・ダッシュ用）
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

// 待機・移動中に共通の入力によるステート遷移
// 遷移した場合は true を返す
bool PlayerState::TryCommonTransition(Player* player)
{
	// Shift：武器装備中は回避、未装備ならダッシュ
	if (IsKeyDown(VK_SHIFT))
	{
		if (player->GetWeaponState() == WeaponState::Equipped)
		{
			player->ChangeState(std::make_shared<PlayerStateEvade>());
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateDash>());
		}
		return true;
	}

	// ジャンプ
	if (player->CheckJumpInput() && player->IsOnGround())
	{
		player->ChangeState(std::make_shared<PlayerStateJump>());
		return true;
	}

	// 武器の抜刀・納刀切り替え
	if (player->CheckEquipInput())
	{
		bool toCombat = !player->IsCombatMode();
		player->SetCombatMode(toCombat);
		if (toCombat)	player->ChangeState(std::make_shared<PlayerStateEquip>());
		else			player->ChangeState(std::make_shared<PlayerStateUnequip>());
		return true;
	}

	// 攻撃
	if (player->GetWeaponState() == WeaponState::Equipped && player->CheckAttackInput())
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(player->GetCurrentAttackType(), 1));
		return true;
	}

	return false;
}

void PlayerStateIdle::ChangeState(Player* player)
{
	player->ChangeAnimation(player->GetWeaponAnimName("Idle"), true);
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateIdle::Update(Player* player)
{
	if (TryCommonTransition(player)) return;

	if (player->CheckMoveInput())
	{
		player->ChangeState(std::make_shared<PlayerStateRun>());
	}
}

void PlayerStateRun::ChangeState(Player* player)
{
	player->ChangeAnimation(player->GetWeaponAnimName("Run"), true);
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateRun::Update(Player* player)
{
	if (TryCommonTransition(player)) return;

	if (!player->MoveProcess())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateEquip::ChangeState(Player* player)
{
	player->ChangeAnimation("Equip", false);
	player->SetAnimationSpeed(1.3f);
}

void PlayerStateEquip::Update(Player* player)
{
	if (player->GetAnimTime() >= player->GetEquipFrame()) {
		player->SetWeaponState(WeaponState::Equipped);
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateUnequip::ChangeState(Player* player)
{
	player->ChangeAnimation("Unequip", false);
	player->SetAnimationSpeed(1.0f);
}

void PlayerStateUnequip::Update(Player* player)
{
	if (player->GetAnimTime() >= player->GetUnequipFrame()) {
		player->SetWeaponState(WeaponState::Sheathed);
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

float PlayerStateComboAttack::GetCancelFrame() const
{
	// 04シリーズの溜め攻撃
	if (IsSplitAttack())
	{
		if (m_subStep == SubStepStart)	return 999.0f;	// Startはキャンセル不可
		if (m_subStep == SubStepLoop)	return 0.0f;	// LoopはいつでもEndへ行ける
		if (m_subStep == SubStepEnd)	return 30.0f;	// Endは落ちる前に4_4へ！
	}

	// 02シリーズ（空中の繋がり）
	if (m_comboType == 2 && m_comboStep == 4) return 30.0f;		// 落ちる前（空中）のフレームを指定
	if (m_comboType == 2 && m_comboStep == 5) return 999.0f;	// 5段目は最後まで再生

	// 03シリーズ
	if (m_comboType == 3 && m_comboStep == 3) return 30.0f;

	// 05シリーズ
	if (m_comboType == 5 && m_comboStep == 2) return 25.0f;
	if (m_comboType == 5 && m_comboStep == 3) return 30.0f;

	// 基本は20フレーム目以降ならクリックで次へ行ける
	return 20.0f;
}

std::string PlayerStateComboAttack::GetAnimName() const
{
	if (IsSplitAttack())
	{
		if (m_subStep == SubStepStart)	return "Attack_4_3_Start";
		if (m_subStep == SubStepLoop)	return "Attack_4_3_Loop";
		if (m_subStep == SubStepEnd)	return "Attack_4_3_End";
	}
	return "Attack_" + std::to_string(m_comboType) + "_" + std::to_string(m_comboStep);
}

bool PlayerStateComboAttack::IsRootMotionLocked() const
{
	return (m_comboType == 2 && m_comboStep == 4) ||
		(m_comboType == 2 && m_comboStep == 5) ||
		IsSplitAttack() ||
		(m_comboType == 5 && m_comboStep == 2) ||
		(m_comboType == 5 && m_comboStep == 3);
}

void PlayerStateComboAttack::ChangeToNextAttack(Player* player) const
{
	if (m_comboStep < kMaxComboStep)
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(m_comboType, m_comboStep + 1));
	}
	else
	{
		// 5段目まで打ち終わった
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateComboAttack::ChangeState(Player* player)
{
	m_effect = std::make_shared<KdEffekseerObject>();
	m_isCritical = (KdRandom::GetInt(0, 99) < player->GetCritRate());

	if (IsSplitAttack() && m_subStep == SubStepNone) {
		m_subStep = SubStepStart;
	}

	bool isLoop = (IsSplitAttack() && m_subStep == SubStepLoop);
	player->ChangeAnimation(GetAnimName(), isLoop, true, 5.0f);

	player->SetAnimationSpeed(1.0f);
	player->SetWeaponState(WeaponState::Equipped);

	// マネージャーの変数を倍率としてセット。一部の攻撃はXZの座標移動を完全にロック
	player->SetUseRootMotion(!IsRootMotionLocked());
	player->SetRootMotionScale(PlayerParamManager::Instance().m_attackRootScale);
}

void PlayerStateComboAttack::Update(Player* player)
{
	player->ExecJump(0.0f);

	// マウスクリックされたら次の攻撃を「予約」する
	if (player->CheckAttackInput())
	{
		m_nextAttackReserved = true;
	}
	
	// 攻撃タイプ04のLoopは一定時間でEndへ
	if (IsSplitAttack() && m_subStep == SubStepLoop)
	{
		m_loopTimer++;
		if (m_loopTimer >= kChargeLoopFrame)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepEnd, m_nextAttackReserved));
			return;
		}
	}

	float animTime = player->GetAnimTime();

	// 攻撃判定と剣の軌跡
	if (animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame)
	{
		// 左右の剣の軌跡を伸ばす（攻撃していないフレームはPlayer側で徐々に消える）
		player->AddSwordTrailPoints();

		if (!m_isHit)
		{
			bool hitR = player->AttackOBB(player->GetSwordMatrixR(), m_isCritical);
			bool hitL = player->AttackOBB(player->GetSwordMatrixL(), m_isCritical);

			// 右手か左手どちらかが敵に当たったらヒットストップ
			if (hitR || hitL)
			{
				player->SetHitStopTimer(1);
				//m_isHit = true;				// 「当てた」と記録する（もう判定しない）
			}
		}

		/*
		if (!m_isEffect)
		{
			Math::Vector3 effectPos = player->GetPos() + Math::Vector3(0.0f, 1.0f, 1.0f);
			std::shared_ptr<Effect> effect = std::make_shared<Effect>();
			effect->Init();

			// 今何段目の攻撃かによって、出すエフェクトファイルを変える！
			if (m_comboStep == 1)      effect->SetEffect("_Sword1-1.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 2) effect->SetEffect("_Sword1-2.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 3) effect->SetEffect("_Sword1-3.efkefc", effectPos, 1.0f);
			else if (m_comboStep == 4) effect->SetEffect("_Sword1-4.efkefc", effectPos, 1.0f);

			SceneManager::Instance().AddObject(effect);
			m_isEffect = true;
		}
		*/
	}

	// 予約があり、キャンセル可能フレームを超えたら次へ滑らかに移行
	if (m_nextAttackReserved && animTime >= GetCancelFrame() && !player->IsAnimEnd())
	{
		if (IsSplitAttack() && m_subStep == SubStepLoop)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepEnd, true));
			return;
		}
		if (m_comboStep < kMaxComboStep)
		{
			ChangeToNextAttack(player);
			return;
		}
	}

	if (!player->IsAnimEnd()) return;

	// 4_3_Start が最後まで終わったら、自動で Loop に繋ぐ
	if (IsSplitAttack() && m_subStep == SubStepStart)
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepLoop));
		return;
	}

	// 予約があれば次の段へ、なければIdleへ（4_3_End の次は 4_4）
	if (m_nextAttackReserved)
	{
		ChangeToNextAttack(player);
	}
	else
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateEvade::ChangeState(Player* player)
{
	player->ChangeAnimation("Evade", false);
	//player->SetInvincibleTimer(10);
	player->SetAnimationSpeed(1.55f);
}

void PlayerStateEvade::Update(Player* player)
{
	MoveForwardWithDecay(player, PlayerParamManager::Instance().m_evadeSpeed, kEvadeMoveFrame, false);

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateDash::ChangeState(Player* player)
{
	player->ChangeAnimation("Dash", false);
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateDash::Update(Player* player)
{
	MoveForwardWithDecay(player, PlayerParamManager::Instance().m_dashSpeed, kEvadeMoveFrame, true);

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateJump::ChangeState(Player* player)
{
	m_jumpPhase = JumpPhaseAir;
	m_jumpCount = 1;
	m_isFalling = false;
	m_isLandingAnimStarted = false;

	player->ExecJump(kJumpPower);
	player->ChangeAnimation(player->GetWeaponAnimName("JumpStart"), false, true);
	player->SetAnimationSpeed(1.0f);
	player->SetUseRootMotion(false);
}

void PlayerStateJump::Update(Player* player)
{
	if (m_jumpPhase != JumpPhaseLanding)
	{
		player->MoveProcess();
	}

	bool isFallingNow = player->GetGravity() > kFallingGravity;

	// --------------------------------------------------
	if (m_jumpPhase == JumpPhaseAir) // 【空中】
	{
		// ジャンプ開始アニメが終わった・落下し始めたら空中ループへ
		if (!m_isLandingAnimStarted && (player->IsAnimEnd() || isFallingNow) && player->GetAnimTime() > 0.0f)
		{
			player->ChangeAnimation(player->GetWeaponAnimName("JumpLoop"), true);
		}

		//// 2段ジャンプの入力検知
		//if (m_jumpCount < 2 && player->CheckJumpInput())
		//{
		//	m_jumpCount++;
		//	player->ExecJump(0.2f); 
		//	player->ChangeAnimation("JumpStart", false, true); 
		//	m_isFalling = false;
		//}

		if (isFallingNow)
		{
			m_isFalling = true;
		}

		if (m_isFalling && !isFallingNow)
		{
			// 落下後に着地した
			// 予測が間に合わなかった場合（高い足場に着地した等）は、両足が接地しているフレームから再生する
			if (!m_isLandingAnimStarted)
			{
				StartLandingAnim(player, 0);
			}
			m_jumpPhase = JumpPhaseLanding;
		}
		else if (m_isFalling && !m_isLandingAnimStarted)
		{
			// 落下中：着地の瞬間に両足が接地しているよう、着地アニメを前倒しで開始する
			TryStartLandingAnim(player);
		}
	}

	// --------------------------------------------------
	else if (m_jumpPhase == JumpPhaseLanding) // 【着地硬直】
	{
		if (player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>());
			return;
		}

		if (player->IsAnimEnd())
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
	}
}

void PlayerStateJump::TryStartLandingAnim(Player* player)
{
	int framesToLand = player->PredictLandingFrames(kMaxLandingPredictFrame);
	if (framesToLand < 0) return;

	// 着地までに再生するフレーム数が、接地フレームより手前で収まるなら開始する
	float landFrame = player->GetLandingFrame(player->GetWeaponAnimName("JumpEnd"));
	if (static_cast<float>(std::max(framesToLand - 1, 0)) <= landFrame)
	{
		StartLandingAnim(player, framesToLand);
	}
}

void PlayerStateJump::StartLandingAnim(Player* player, int framesToLand)
{
	std::string animName = player->GetWeaponAnimName("JumpEnd");
	float landFrame = player->GetLandingFrame(animName);

	// 次のフレームから再生されるので、着地フレームで landFrame が表示されるよう逆算する
	float waitFrames = static_cast<float>(std::max(framesToLand - 1, 0));
	float startTime = std::max(landFrame - waitFrames, 0.0f);

	// 着地までに余裕があれば空中ループから滑らかにつなぐ（着地時にはブレンドが終わっているように）
	float blendFrame = (waitFrames >= kLandingBlendFrame) ? kLandingBlendFrame : 0.0f;

	player->ChangeAnimation(animName, false, true, blendFrame);
	player->SetAnimTime(startTime);
	m_isLandingAnimStarted = true;
}

void PlayerStateDamage::ChangeState(Player* player)
{
	// 仰け反るようなダメージモーションを再生
	player->ChangeAnimation("Damage", false, true);
	player->SetAnimationSpeed(1.0f);
}

void PlayerStateDamage::Update(Player* player)
{
	// 少しの間、後ろへノックバック
	if (player->GetAnimTime() < kDamageKnockbackFrame)
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
	player->SetAnimationSpeed(1.0f);
}

void PlayerStateDead::Update(Player* player)
{
	// 死亡モーションの後はゲームオーバー画面へ（GameScene側で判定）
	// 倒れた後に動かないよう、ここでは何もしない
}