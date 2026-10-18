#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../Scene/SceneManager.h"

using InputHelper::IsKeyDown;

namespace
{
	// 攻撃判定を出すアニメーションフレームの範囲
	constexpr float kAttackHitStartFrame	= 10.0f;
	constexpr float kAttackHitEndFrame		= 45.0f;
	constexpr int	kChargeLoopFrame		= 90;		// 4-3 Loop の持続フレーム
	constexpr int	kMaxComboStep			= 5;

	// 回避・被弾で移動するフレーム数
	constexpr float kEvadeMoveFrame			= 25.0f;
	constexpr float kDamageKnockbackFrame	= 15.0f;
	constexpr float kDamageKnockbackSpeed	= 0.06f;

	// ジャンプ
	constexpr float kJumpPower				= 0.25f;
	constexpr float kFallingGravity			= 0.011f;	// これを超えたら落下中とみなす
	constexpr int	kMaxLandingPredictFrame	= 120;		// 着地予測を行う最大フレーム数
	constexpr float kLandingBlendFrame		= 3.0f;		// 空中ループ→着地アニメのブレンドフレーム数
	constexpr float kJumpStartBlendFrame	= 4.0f;		// 待機・走り→ジャンプ開始のブレンドフレーム数
	constexpr float kJumpWindupFrames		= 6.0f;		// ジャンプの「ため」を見せるフレーム数（アニメーションのためが長い時は、途中から再生する）
	constexpr int	kMaxLandingLeadFrames	= 12;		// 着地アニメーションを、着地の何フレーム前まで早めに始めてよいか
	constexpr float kAirLoopBlendFrame		= 8.0f;		// ジャンプ開始→空中ループのブレンドフレーム数（パッと切り替わらないように）
	constexpr float kFallGravityRate		= 1.5f;		// 落ちる時の重力の倍率（上がる時より速く落ちて、ふわふわしない）

	// 空中での移動
	constexpr float kAirMinSpeed			= 0.06f;	// その場ジャンプから入力した時の速さ（1フレームの移動量）
	constexpr float kAirMaxSpeed			= 0.3f;		// 空中の速さの上限
	constexpr float kAirSteerRate			= 0.08f;	// 入力の方向へ曲がる強さ（0～1）
	constexpr float kAirDrag				= 0.99f;	// 入力が無い時の減速（1フレームごとに掛ける。空中で急に止まらないよう弱め）
	constexpr float kAirMaxTurnAngle		= 6.0f;		// 空中で1フレームに向きを変えられる角度（度）
	constexpr float kAirFaceMinSpeed		= 0.02f;	// これより遅い時は向きを変えない（その場ジャンプで体がぶれないように）
	constexpr float kLandingSlideDrag		= 0.75f;	// 着地した後、勢いを少しだけ残して止まる（1フレームごとに掛ける）

	// 走り（ルートモーション）
	constexpr float kRunBlendFrame			= 4.0f;		// 待機⇔走りのブレンドフレーム数（小さいほど素早く切り替わる）
	constexpr float kAwakeningRunSpeedRate	= 1.7f;		// 覚醒中は、普段の走る速さの何倍にするか
	constexpr float kRunAnimSpeedShare		= 0.6f;		// 速くする分のうち、アニメーションの再生速度で上げる割合

	// 回避・着地エフェクトの大きさ
	constexpr float kEvadeEffectScale		= 1.0f;
	constexpr float kLandingEffectScale		= 1.0f;

	// プレイヤーは常に剣を抜いているので、剣を持った（Combat_）アニメーションを使う
	const std::string kIdleAnimName			= "Combat_Idle";
	const std::string kRunAnimName			= "Combat_Run";
	const std::string kJumpStartAnimName	= "Combat_JumpStart";
	const std::string kJumpLoopAnimName		= "Combat_JumpLoop";
	const std::string kJumpEndAnimName		= "Combat_JumpEnd";

	// アビリティ（ATBコマンドメニューで選ぶ攻撃04・05。使うゲージの量・CTは Player.cpp の kAbilityInfos）
	// アビリティの攻撃は、ボタンを押さなくても最後の段まで自動で出る
	constexpr float kAbilityDamageRate		= 1.5f;		// 普通の攻撃の何倍のダメージか

	// ガード（止まっている時に Shift を押している間）
	// アニメーション：Sequence2/08_Hit/14_Block の Block_Start / Loop / Hit / End
	const std::string kGuardStartAnimName	= "GuardStart";
	const std::string kGuardLoopAnimName	= "GuardLoop";
	const std::string kGuardHitAnimName		= "GuardHit";
	const std::string kGuardEndAnimName		= "GuardEnd";
	constexpr float kGuardBlendFrame		= 4.0f;		// ガードのアニメーションどうしのブレンドフレーム数
	constexpr float kGuardStartAnimSpeed	= 1.5f;		// 構えるのは素早く（すぐ守れるように）
	constexpr float kGuardEndCancelRate		= 0.3f;		// 構えを解くアニメーションがこの割合まで進んだら、次の行動に移れる
	constexpr float kGuardWalkBlendFrame	= 6.0f;		// ガード歩きの向きが変わった時のブレンドフレーム数
	constexpr float kGuardTurnAngle			= 12.0f;	// ガード中にカメラの方へ向きを変える速さ（度/フレーム）
	const std::string kGuardBreakAnimName	= "GuardBreak";

	// ダッシュ（移動中に Shift を押している間）
	constexpr float kDashSpeedRate			= 1.6f;		// 走る速さの何倍か

	// 回避
	constexpr float kEvadeAnimSpeed			= 1.8f;		// 回避の再生速度（移動距離は同じまま、素早く転がる）
	constexpr float kEvadeCancelRate		= 0.55f;	// 回避のアニメーションのこの割合まで進んだら、次の行動（攻撃・回避・移動）に移れる
	constexpr float kEvadeBlendFrame		= 3.0f;		// 前の動き→回避のブレンドフレーム数

	// 攻撃
	constexpr float kAttackAnimSpeed		= 1.15f;	// 攻撃の再生速度（少し速くして、もっさりしないようにする）
	constexpr float kAttack5_3AnimSpeed		= 0.9f;		// 攻撃05の3段目だけは、少しゆっくり再生する
	constexpr float kAttackEvadeCancelFrame	= 12.0f;	// 攻撃のこのフレーム以降なら、回避でキャンセルできる
	constexpr float kAttackRecoveryFrame	= 6.0f;		// 次の攻撃に移れるフレームから、さらにこれだけたったら、移動・ジャンプで攻撃の後のすきをキャンセルできる

	// 回避した時に、パニッシャーからオペレーターに戻すか（FF7リメイクのクラウドと同じ動き）
	// false にすると、回避してもモードはそのまま
	constexpr bool	kEvadeReturnsToOperator	= true;

	// 着地
	constexpr float kLandingCancelFrame		= 4.0f;		// 着地してからこのフレーム数がたったら、攻撃・回避・ジャンプに移れる
}

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

	// ジャンプ（空中で押した入力は、着地するまで残しておく）
	if (player->IsOnGround() && player->CheckJumpInput())
	{
		player->ChangeState(std::make_shared<PlayerStateJump>());
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
		// 移動（Shift を押していればダッシュ）
		player->ChangeState(std::make_shared<PlayerStateRun>());
	}
	else if (player->IsShiftDown() && player->CanGuard())
	{
		// 止まっている時に Shift：ガード（構えた後は、移動キーでガードしたまま歩ける）
		player->ChangeState(std::make_shared<PlayerStateGuard>());
	}
}

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
		// もう一度 Shift を押したら、すぐ構え直す
		if (player->IsShiftDown() && player->CanGuard() && !player->CheckMoveInput())
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
		// 構え終わったら、構えたままのループへ（Shift を離していたら、そのまま構えを解く）
		if (player->IsAnimEnd())
		{
			ChangePhase(player, player->IsShiftDown() ? Phase::Loop : Phase::End);
		}
		break;

	case Phase::Loop:
		// Shift を離したら構えを解く（移動キーを押していれば、そのまま走り出す）
		if (!player->IsShiftDown())
		{
			if (player->CheckMoveInput())	player->ChangeState(std::make_shared<PlayerStateRun>());
			else							ChangePhase(player, Phase::End);
			return;
		}

		// 構えたまま歩く・止まる
		UpdateGuardMove(player);
		break;

	case Phase::Hit:
		// 受け止める動きが終わったら、Shift を押していれば構えたまま、離していれば構えを解く
		if (player->IsAnimEnd())
		{
			ChangePhase(player, player->IsShiftDown() ? Phase::Loop : Phase::End);
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

void PlayerStateRun::ChangeState(Player* player)
{
	player->ChangeAnimation(kRunAnimName, true, false, kRunBlendFrame);
	player->SetAnimationSpeed(1.0f);

	// 走りのアニメーションの動きで前へ進む（足の動きと移動の速さが一致するので、足が地面を滑らない）
	UseRootMotion(player);
}

void PlayerStateRun::Update(Player* player)
{
	if (TryCommonTransition(player)) return;

	// アニメーションに前へ進む動きがあれば、その動きで移動する
	// （その場で足踏みするだけのアニメーションなら、プログラムで移動する）
	bool isAnimMotion = player->CurrentAnimHasHorizontalMotion();

	// 走る速さ（アニメーションの動きで進む時は、再生速度と移動倍率の両方で速くする）
	//   再生速度だけを上げると足の動きがせわしなくなり、移動倍率だけを上げると足が滑って見えるので、
	//   速くする分の 6 割を再生速度、残りを移動倍率で上げる
	//   例：倍率 1.5 → 再生速度 1.3 × 移動倍率 約1.15
	if (isAnimMotion)
	{
		float speedRate = Player::s_runSpeedRate;
		if (player->IsAwakening()) speedRate *= kAwakeningRunSpeedRate;	// 覚醒中はさらに速く
		speedRate *= player->GetModeMoveRate();								// パニッシャーの時は遅く
		if (player->IsShiftDown()) speedRate *= kDashSpeedRate;				// Shift を押している間はダッシュ

		float animSpeed = 1.0f + (speedRate - 1.0f) * kRunAnimSpeedShare;
		player->SetAnimationSpeed(animSpeed);
		player->SetRootMotionScale(speedRate / animSpeed);
	}

	// アニメーションの動きで進む時は、入力方向へ向きを変えるだけ
	if (!player->MoveProcess(!isAnimMotion))
	{
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

void PlayerStateComboAttack::ChangeToNextAttack(Player* player) const
{
	// コンボの途中でバトルモードを切り替えた時は、今のモードの攻撃を1段目から出す
	// （前は切り替える前の攻撃のまま続いたので、オペレーターでも攻撃02が出てしまっていた）
	// ※アビリティ（ATBコマンド）の攻撃は、モードに関係なく最後まで出す
	if (!m_isAbility && player->GetCurrentAttackType() != m_comboType)
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(player->GetCurrentAttackType(), 1));
		return;
	}

	if (m_comboStep < kMaxComboStep)
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(m_comboType, m_comboStep + 1, SubStepNone, false, m_isAbility));
	}
	else
	{
		// 5段目まで打ち終わった
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

const std::string& PlayerStateComboAttack::GetSlashEffectName() const
{
	// 4-3（溜め攻撃）：タイプ4の斬撃
	if (IsSplitAttack())
	{
		return EffectName::ComboSlash4;
	}

	switch (m_comboType)
	{
	case 1:
		// 攻撃タイプ1は段数ごとに専用のエフェクト
		switch (m_comboStep)
		{
		case 1:	return EffectName::ComboSlash1_1;
		case 2:	return EffectName::ComboSlash1_2;
		case 3:	return EffectName::ComboSlash1_3;
		case 4:	return EffectName::ComboSlash1_4;
		default: break;
		}
		break;

	case 4:
		return EffectName::ComboSlash4;

	case 5:
		if (m_comboStep == 4) return EffectName::ComboSlash5_4;
		break;

	default:
		break;
	}

	return EffectName::ComboSlashDefault;
}

bool PlayerStateComboAttack::IsTrailFrame(const Player* player, float animTime) const
{
	// 溜め攻撃（4-3）：Start の振り始めから、End で剣を地面に叩きつけるまで、途切れずに出す
	if (IsSplitAttack())
	{
		switch (m_subStep)
		{
		case SubStepStart:	return animTime >= kAttackHitStartFrame;
		case SubStepLoop:	return true;
		case SubStepEnd:
		{
			float slamFrame = player->GetSwordSlamFrame();
			return animTime <= ((slamFrame >= 0.0f) ? slamFrame : kAttackHitEndFrame);
		}
		default:			break;
		}
	}

	// 普通の攻撃：攻撃判定のフレームの間
	return animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame;
}

const std::string& PlayerStateComboAttack::GetHitEffectName() const
{
	return GetSlashEffectName();
}

void PlayerStateComboAttack::ChangeState(Player* player)
{
	m_isCritical = (KdRandom::GetInt(0, 99) < player->GetCritRate());

	if (IsSplitAttack() && m_subStep == SubStepNone) {
		m_subStep = SubStepStart;
	}

	bool isLoop = (IsSplitAttack() && m_subStep == SubStepLoop);
	player->ChangeAnimation(GetAnimName(), isLoop, true, 5.0f);

	// ロックオン中は、振り始める時に敵の方を向く（振っている途中は向きを変えない）
	player->FaceLockTarget();

	// 再生速度（05の3段目だけ少しゆっくり）
	bool isSlowAttack = (m_comboType == 5 && m_comboStep == 3);
	player->SetAnimationSpeed(isSlowAttack ? kAttack5_3AnimSpeed : kAttackAnimSpeed);

	// アビリティの攻撃中は、当ててもATBゲージがたまらない
	player->SetAbilityAttack(m_isAbility);

	// マネージャーの変数を倍率としてセット。一部の攻撃はXZの座標移動を完全にロック
	// アニメーションの踏み込みどおりに動く
	// （倍率は 1.0。小さくすると足が地面を滑って見え、0にすると技の途中で剣を振る位置がずれる）
	UseRootMotion(player);
}

void PlayerStateComboAttack::Update(Player* player)
{
	player->ExecJump(0.0f);

	// マウスクリックされたら次の攻撃を「予約」する
	// （アビリティの攻撃は、押さなくても最後の段まで自動で出す）
	if (player->CheckAttackInput() || m_isAbility)
	{
		m_nextAttackReserved = true;
	}

	// 右クリック：攻撃の途中でも、回避でキャンセルできる（攻撃の出始めは除く）
	if (player->GetAnimTime() >= kAttackEvadeCancelFrame && player->CheckEvadeInput())
	{
		StartEvade(player);
		return;
	}


	// 攻撃タイプ04のLoopは一定時間でEndへ
	if (IsSplitAttack() && m_subStep == SubStepLoop)
	{
		m_loopTimer++;
		if (m_loopTimer >= kChargeLoopFrame)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepEnd, m_nextAttackReserved, m_isAbility));
			return;
		}
	}

	float animTime = player->GetAnimTime();

	// 剣の軌跡（攻撃していないフレームはPlayer側で徐々に消える）
	if (IsTrailFrame(player, animTime))
	{
		player->AddSwordTrailPoints();
	}

	// 攻撃判定
	if (animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame)
	{
		// 当たったら、攻撃の種類・段数ごとのエフェクトをその位置に出す
		// （ヒットストップは AttackOBB の中で、すべての攻撃に同じルールでかかる）
		// （敵は当たると少しの間無敵になるので、毎フレーム判定しても何度も当たらない）
		float damageRate = m_isAbility ? kAbilityDamageRate : 1.0f;
		player->AttackOBB(player->GetSwordMatrixR(), m_isCritical, GetHitEffectName(), damageRate);
		player->AttackOBB(player->GetSwordMatrixL(), m_isCritical, GetHitEffectName(), damageRate);
	}

	// 予約があり、キャンセル可能フレームを超えたら次へ滑らかに移行
	// （アビリティの時は、溜め攻撃のループを最後まで見せる）
	bool isAbilityChargeLoop = m_isAbility && IsSplitAttack() && m_subStep == SubStepLoop;
	if (m_nextAttackReserved && !isAbilityChargeLoop && animTime >= GetCancelFrame() && !player->IsAnimEnd())
	{
		if (IsSplitAttack() && m_subStep == SubStepLoop)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepEnd, true, m_isAbility));
			return;
		}
		if (m_comboStep < kMaxComboStep)
		{
			ChangeToNextAttack(player);
			return;
		}
	}

	// 次の攻撃の予約が無く、少したったら、移動・ジャンプで攻撃の後のすきをキャンセルできる
	// （アビリティ・溜め攻撃の途中・最後まで再生する攻撃は除く）
	float cancelFrame = GetCancelFrame();
	bool canRecoveryCancel = !m_isAbility && !m_nextAttackReserved && !IsSplitAttack()
		&& cancelFrame < kAttackHitEndFrame && animTime >= cancelFrame + kAttackRecoveryFrame;
	if (canRecoveryCancel)
	{
		if (player->IsOnGround() && player->CheckJumpInput())
		{
			player->ChangeState(std::make_shared<PlayerStateJump>());
			return;
		}
		if (player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>());
			return;
		}
	}

	if (!player->IsAnimEnd()) return;

	// 4_3_Start が最後まで終わったら、自動で Loop に繋ぐ
	if (IsSplitAttack() && m_subStep == SubStepStart)
	{
		player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepLoop, false, m_isAbility));
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
	// 移動キーを押していれば、その方向へすぐ向いてから転がる
	// （前は今向いている方向にしか回避できず、思った方向へ逃げられなかった）
	Math::Vector3 inputDir;
	if (player->GetMoveInputDir(inputDir))
	{
		player->TurnToward(inputDir, 180.0f);
	}

	player->ChangeAnimation("Evade", false, true, kEvadeBlendFrame);
	UseRootMotion(player);
	EffectPlayer::Play(EffectName::Evade, player->GetPos(), kEvadeEffectScale);
	//player->SetInvincibleTimer(10);
	player->SetAnimationSpeed(kEvadeAnimSpeed);
}

void PlayerStateEvade::Update(Player* player)
{
	// アニメーションに前へ進む動きが無い場合だけ、プログラムで前へ進む
	if (!player->CurrentAnimHasHorizontalMotion())
	{
		MoveForwardWithDecay(player, PlayerParamManager::Instance().m_evadeSpeed, kEvadeMoveFrame, false);
	}

	// 転がり終わり（起き上がる所）は、次の行動でキャンセルできる
	float animLength = player->GetAnimLength();
	bool canCancel = (animLength > 0.0f) && (player->GetAnimTime() >= animLength * kEvadeCancelRate);
	if (canCancel)
	{
		if (TryCommonTransition(player)) return;

		if (player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>());
			return;
		}
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateJump::ChangeState(Player* player)
{
	m_jumpPhase = JumpPhaseAir;
	m_isFalling = false;
	m_isLandingAnimStarted = false;

	// アニメーションの踏み切り（足が地面から離れる）フレームで跳ぶ
	// （すぐ跳ぶと、しゃがんで力をためている姿勢のまま宙に浮いてしまうため）
	const std::string& startAnimName = kJumpStartAnimName;
	m_takeoffFrame = player->GetTakeoffFrame(startAnimName);
	m_isTakeoffDone = false;

	player->ChangeAnimation(startAnimName, false, true, kJumpStartBlendFrame);
	player->SetAnimationSpeed(1.0f);

	// アニメーションの「ため」（しゃがんで力をためる動き）が長いと、ボタンを押してから跳ぶまで待たされるので、
	// 踏み切りの kJumpWindupFrames フレーム前から再生する
	if (m_takeoffFrame > kJumpWindupFrames)
	{
		player->SetAnimTime(m_takeoffFrame - kJumpWindupFrames);
	}

	// 空中の移動は「踏み切った時の勢い」で行うので、アニメーションの動きでは移動しない
	// （腰はキャラの位置に固定し、アニメーションの切り替えで体がずれないようにする）
	UseRootMotion(player);
	player->SetRootMotionScale(0.0f);

	// 走っていた勢いをそのまま引き継ぐ
	m_airVelocity = player->GetLastMoveDelta();
	m_airSpeed = std::clamp(m_airVelocity.Length(), kAirMinSpeed, kAirMaxSpeed);

	// 落ちる時は速く落とす（着地の予測もこの重力で計算される）
	player->SetFallGravityRate(kFallGravityRate);

	// 踏み切りを待たないアニメーションなら、すぐ跳ぶ
	if (m_takeoffFrame <= 0.0f)
	{
		Takeoff(player);
	}
}

void PlayerStateJump::Takeoff(Player* player)
{
	player->ExecJump(kJumpPower);
	m_isTakeoffDone = true;

	// ここから先は、アニメーションで腰が空中ループより高く上がる分を止める
	// （高さはプログラムのジャンプで出しているので、アニメーションでも上がると2重に上下して見えるため）
	player->SetAirPelvisMaxY(player->GetAirPelvisMaxY());
}

void PlayerStateJump::UpdateAirMove(Player* player)
{
	Math::Vector3 inputDir;
	if (player->GetMoveInputDir(inputDir))
	{
		// 速さを保ったまま、入力の方向へ少しずつ曲がる
		m_airVelocity = Math::Vector3::Lerp(m_airVelocity, inputDir * m_airSpeed, kAirSteerRate);
	}
	else
	{
		// 入力が無くても、勢いはほとんど残す（空中で急にブレーキがかからないように）
		m_airVelocity *= kAirDrag;
	}

	// 体の向きは「入力の方向」ではなく「実際に進んでいる方向」に合わせる
	// （前は入力の方向へすぐ向いていたので、体の向きと進む方向がずれて、空中で横滑りして見えていた）
	if (m_airVelocity.LengthSquared() > kAirFaceMinSpeed * kAirFaceMinSpeed)
	{
		Math::Vector3 moveDir = m_airVelocity;
		moveDir.Normalize();
		player->TurnToward(moveDir, kAirMaxTurnAngle);
	}

	player->SetPos(player->GetPos() + m_airVelocity);
}

void PlayerStateJump::Update(Player* player)
{
	if (m_jumpPhase != JumpPhaseLanding)
	{
		// 空中での移動（踏み切る前も、走っていた勢いのまま進む）
		UpdateAirMove(player);
	}

	// 踏み切りのフレームになったら跳ぶ（それまでは地面にいるので、着地の判定もしない）
	if (!m_isTakeoffDone)
	{
		if (player->GetAnimTime() >= m_takeoffFrame)
		{
			Takeoff(player);
		}
		return;
	}

	bool isFallingNow = player->GetGravity() > kFallingGravity;

	// --------------------------------------------------
	if (m_jumpPhase == JumpPhaseAir) // 【空中】
	{
		// ジャンプ開始アニメが終わった・落下し始めたら空中ループへ
		// （ジャンプ開始アニメの最後のポーズは、空中ループの最初のポーズと同じなので、そのままつながる）
		if (!m_isLandingAnimStarted && (player->IsAnimEnd() || isFallingNow) && player->GetAnimTime() > 0.0f)
		{
			player->ChangeAnimation(kJumpLoopAnimName, true, false, kAirLoopBlendFrame);
		}

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

			// 地面に着いたので、腰の高さはアニメーションどおりに戻す（着地でひざを曲げる動きのため）
			player->SetAirPelvisMaxY(-1.0f);

			// 着地エフェクト（足元に出す）
			EffectPlayer::Play(EffectName::Landing, player->GetPos(), kLandingEffectScale);
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
		// 空中の勢いを少しだけ残して止まる（着地の瞬間にピタッと止まらないように）
		m_airVelocity *= kLandingSlideDrag;
		player->SetPos(player->GetPos() + m_airVelocity);

		// 着地して少したったら、しゃがみ込みの途中でも攻撃・回避・ジャンプに移れる
		m_landingTimer++;
		if (m_landingTimer >= kLandingCancelFrame && TryCommonTransition(player)) return;

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
	// （ただし早すぎると空中で着地の動きを始めてしまうので、kMaxLandingLeadFrames フレーム前まで）
	if (framesToLand > kMaxLandingLeadFrames) return;

	float landFrame = player->GetLandingFrame(kJumpEndAnimName);
	if (static_cast<float>(std::max(framesToLand - 1, 0)) <= landFrame)
	{
		StartLandingAnim(player, framesToLand);
	}
}

void PlayerStateJump::StartLandingAnim(Player* player, int framesToLand)
{
	const std::string& animName = kJumpEndAnimName;
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