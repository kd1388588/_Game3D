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

	// 空中での移動
	constexpr float kAirMinSpeed			= 0.06f;	// その場ジャンプから入力した時の速さ（1フレームの移動量）
	constexpr float kAirMaxSpeed			= 0.3f;		// 空中の速さの上限
	constexpr float kAirSteerRate			= 0.08f;	// 入力の方向へ曲がる強さ（0～1）
	constexpr float kAirDrag				= 0.98f;	// 入力が無い時の減速（1フレームごとに掛ける）

	// 走り（ルートモーション）
	constexpr float kRunBlendFrame			= 6.0f;		// 待機⇔走りのブレンドフレーム数
	constexpr float kAwakeningRunSpeedRate	= 1.7f;		// 覚醒中は、普段の走る速さの何倍にするか
	constexpr float kRunAnimSpeedShare		= 0.6f;		// 速くする分のうち、アニメーションの再生速度で上げる割合

	// 斬撃エフェクト（プレイヤーの前方・胸の高さあたりに出す）
	constexpr float kSlashEffectForward		= 1.0f;		// 前方へずらす距離
	constexpr float kSlashEffectHeight		= 1.0f;		// 足元からの高さ
	constexpr float kSlashEffectScale		= 1.0f;		// 大きさ

	// 回避・着地エフェクトの大きさ
	constexpr float kEvadeEffectScale		= 1.0f;
	constexpr float kLandingEffectScale		= 1.0f;

	// プレイヤーは常に剣を抜いているので、剣を持った（Combat_）アニメーションを使う
	const std::string kIdleAnimName			= "Combat_Idle";
	const std::string kRunAnimName			= "Combat_Run";
	const std::string kJumpStartAnimName	= "Combat_JumpStart";
	const std::string kJumpLoopAnimName		= "Combat_JumpLoop";
	const std::string kJumpEndAnimName		= "Combat_JumpEnd";

	// アビリティ（ATBゲージを1本使う技）
	// モードごとに出す技（今ある攻撃アニメーションの中から、強そうなものを使っている）
	const std::string kOperatorAbilityAnim	= "Attack_1_4";		// オペレーター：素早い連続斬り
	const std::string kPunisherAbilityAnim	= "Attack_4_4";		// パニッシャー：重い一撃
	constexpr float kAbilityDamageRate		= 2.5f;		// 普通の攻撃の何倍のダメージか
	constexpr int	kAbilityHitStopFrame	= 4;		// 当たった時に止まるフレーム数（普通の攻撃は1）
	constexpr float kAbilityAnimSpeed		= 1.0f;		// 再生速度
	constexpr float kAbilityBlendFrame		= 5.0f;		// 前の動きから技へのブレンドフレーム数
	constexpr float kAbilityEffectScale		= 1.0f;		// 発動エフェクトの大きさ

	// ATBゲージが足りない時のお知らせ（ログに出す）
	void LogAtbShortage()
	{
		KdDebugGUI::Instance().AddLog("[System] ATBゲージが足りません\n");
	}
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

bool PlayerState::TryUseAbility(Player* player)
{
	// ATBゲージを1本使う（足りなければ出さない）
	if (!player->UseAtbBar())
	{
		LogAtbShortage();
		return false;
	}

	player->ChangeState(std::make_shared<PlayerStateAbility>());
	return true;
}

// 待機・移動中に共通の入力によるステート遷移
// 遷移した場合は true を返す
bool PlayerState::TryCommonTransition(Player* player)
{
	// Shift：回避
	// （パニッシャーの時は、オペレーターに戻ってから回避する。FF7リメイクのクラウドと同じ動き）
	if (IsKeyDown(VK_SHIFT))
	{
		if (player->IsPunisherMode())
		{
			player->SetBattleMode(BattleMode::Operator);
		}
		player->ChangeState(std::make_shared<PlayerStateEvade>());
		return true;
	}

	// ジャンプ
	if (player->CheckJumpInput() && player->IsOnGround())
	{
		player->ChangeState(std::make_shared<PlayerStateJump>());
		return true;
	}

	// 右クリック：ATBゲージを1本使ってアビリティ
	if (player->CheckAbilityInput() && TryUseAbility(player))
	{
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
		player->ChangeState(std::make_shared<PlayerStateRun>());
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

const std::string& PlayerStateComboAttack::GetSlashEffectName() const
{
	// 4-3（溜め攻撃）：ループ中はドリル、それ以外はタイプ4の斬撃
	if (IsSplitAttack())
	{
		return (m_subStep == SubStepLoop) ? EffectName::ChargeDrill : EffectName::ComboSlash4;
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
	// 溜め攻撃のループ中はドリル（ループするエフェクト）なので、当たった時はタイプ4の斬撃にする
	if (IsSplitAttack() && m_subStep == SubStepLoop)
	{
		return EffectName::ComboSlash4;
	}
	return GetSlashEffectName();
}

void PlayerStateComboAttack::PlaySlashEffect(Player* player)
{
	if (m_isEffect) return;

	// 斬撃のエフェクトは敵に当たった時に出す（AttackOBB）
	// ここでは溜め攻撃のループ中のドリルだけを、振っている間ずっと出す
	if (!(IsSplitAttack() && m_subStep == SubStepLoop)) return;

	m_effect = EffectPlayer::Play(GetSlashEffectName(),
		player->CreateEffectMatrix(kSlashEffectForward, kSlashEffectHeight, kSlashEffectScale));
	m_isEffect = true;
}

void PlayerStateComboAttack::ChangeState(Player* player)
{
	m_isCritical = (KdRandom::GetInt(0, 99) < player->GetCritRate());

	if (IsSplitAttack() && m_subStep == SubStepNone) {
		m_subStep = SubStepStart;
	}

	bool isLoop = (IsSplitAttack() && m_subStep == SubStepLoop);
	player->ChangeAnimation(GetAnimName(), isLoop, true, 5.0f);

	player->SetAnimationSpeed(1.0f);

	// マネージャーの変数を倍率としてセット。一部の攻撃はXZの座標移動を完全にロック
	// アニメーションの踏み込みどおりに動く
	// （倍率は 1.0。小さくすると足が地面を滑って見え、0にすると技の途中で剣を振る位置がずれる）
	UseRootMotion(player);
}

void PlayerStateComboAttack::Update(Player* player)
{
	player->ExecJump(0.0f);

	// マウスクリックされたら次の攻撃を「予約」する
	if (player->CheckAttackInput())
	{
		m_nextAttackReserved = true;
	}

	// 右クリックされたらアビリティを「予約」する（ATBゲージが1本以上ある時だけ）
	if (player->CheckAbilityInput())
	{
		if (player->GetAtbBarCount() >= 1)	m_abilityReserved = true;
		else								LogAtbShortage();
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

	// 剣の軌跡（攻撃していないフレームはPlayer側で徐々に消える）
	if (IsTrailFrame(player, animTime))
	{
		player->AddSwordTrailPoints();
	}

	// 攻撃判定
	if (animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame)
	{
		if (!m_isHit)
		{
			// 当たったら、攻撃の種類・段数ごとのエフェクトをその位置に出す
			bool hitR = player->AttackOBB(player->GetSwordMatrixR(), m_isCritical, GetHitEffectName());
			bool hitL = player->AttackOBB(player->GetSwordMatrixL(), m_isCritical, GetHitEffectName());

			// 右手か左手どちらかが敵に当たったらヒットストップ
			if (hitR || hitL)
			{
				player->SetHitStopTimer(1);
				//m_isHit = true;				// 「当てた」と記録する（もう判定しない）
			}
		}

		// 溜め攻撃のループ中はドリルのエフェクト
		PlaySlashEffect(player);
	}

	// アビリティの予約があり、キャンセル可能フレームを超えたら、コンボを止めてアビリティを出す
	if (m_abilityReserved && animTime >= GetCancelFrame())
	{
		m_abilityReserved = false;
		if (TryUseAbility(player)) return;
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

	// 最後まで再生してからアビリティの予約を使う（キャンセルできない攻撃の時）
	if (m_abilityReserved && TryUseAbility(player)) return;

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
	UseRootMotion(player);
	EffectPlayer::Play(EffectName::Evade, player->GetPos(), kEvadeEffectScale);
	//player->SetInvincibleTimer(10);
	player->SetAnimationSpeed(1.55f);
}

void PlayerStateEvade::Update(Player* player)
{
	// アニメーションに前へ進む動きが無い場合だけ、プログラムで前へ進む
	if (!player->CurrentAnimHasHorizontalMotion())
	{
		MoveForwardWithDecay(player, PlayerParamManager::Instance().m_evadeSpeed, kEvadeMoveFrame, false);
	}

	if (player->IsAnimEnd()) {
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateAbility::ChangeState(Player* player)
{
	// アビリティの攻撃中は、当ててもATBゲージがたまらない
	player->SetAbilityAttack(true);

	m_isCritical = (KdRandom::GetInt(0, 99) < player->GetCritRate());

	// モードごとの技を出す
	const std::string& animName = player->IsPunisherMode() ? kPunisherAbilityAnim : kOperatorAbilityAnim;
	player->ChangeAnimation(animName, false, true, kAbilityBlendFrame);
	player->SetAnimationSpeed(kAbilityAnimSpeed);
	UseRootMotion(player);

	EffectPlayer::Play(EffectName::AbilityStart, player->GetPos(), kAbilityEffectScale);

	KdDebugGUI::Instance().AddLog("[System] アビリティ発動: %s\n", animName.c_str());
}

void PlayerStateAbility::Update(Player* player)
{
	player->ExecJump(0.0f);

	float animTime = player->GetAnimTime();

	// 攻撃判定のフレームの間は、剣の軌跡を出して攻撃判定を行う
	if (animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame)
	{
		player->AddSwordTrailPoints();

		const std::string& hitEffect = player->IsPunisherMode() ? EffectName::ComboSlash4 : EffectName::ComboSlash1_4;
		bool hitR = player->AttackOBB(player->GetSwordMatrixR(), m_isCritical, hitEffect, kAbilityDamageRate);
		bool hitL = player->AttackOBB(player->GetSwordMatrixL(), m_isCritical, hitEffect, kAbilityDamageRate);

		// 普通の攻撃より長く止めて、重い一撃に見せる
		if (hitR || hitL)
		{
			player->SetHitStopTimer(kAbilityHitStopFrame);
		}
	}

	if (player->IsAnimEnd())
	{
		player->ChangeState(std::make_shared<PlayerStateIdle>());
	}
}

void PlayerStateJump::ChangeState(Player* player)
{
	m_jumpPhase = JumpPhaseAir;
	m_jumpCount = 1;
	m_isFalling = false;
	m_isLandingAnimStarted = false;

	// アニメーションの踏み切り（足が地面から離れる）フレームで跳ぶ
	// （すぐ跳ぶと、しゃがんで力をためている姿勢のまま宙に浮いてしまうため）
	const std::string& startAnimName = kJumpStartAnimName;
	m_takeoffFrame = player->GetTakeoffFrame(startAnimName);
	m_isTakeoffDone = false;
	if (m_takeoffFrame <= 0.0f)
	{
		player->ExecJump(kJumpPower);
		m_isTakeoffDone = true;
	}

	player->ChangeAnimation(startAnimName, false, true);
	player->SetAnimationSpeed(1.0f);

	// 空中の移動は「踏み切った時の勢い」で行うので、アニメーションの動きでは移動しない
	// （腰はキャラの位置に固定し、アニメーションの切り替えで体がずれないようにする）
	UseRootMotion(player);
	player->SetRootMotionScale(0.0f);

	// 走っていた勢いをそのまま引き継ぐ
	m_airVelocity = player->GetLastMoveDelta();
	m_airSpeed = std::clamp(m_airVelocity.Length(), kAirMinSpeed, kAirMaxSpeed);
}

void PlayerStateJump::UpdateAirMove(Player* player)
{
	// 入力の方向へ向きを変える（覚醒の入力もここで受け付ける）
	bool hasInput = player->MoveProcess(false);

	Math::Vector3 inputDir;
	if (hasInput && player->GetMoveInputDir(inputDir))
	{
		// 速さを保ったまま、入力の方向へ少しずつ曲がる
		m_airVelocity = Math::Vector3::Lerp(m_airVelocity, inputDir * m_airSpeed, kAirSteerRate);
	}
	else
	{
		// 入力が無ければ、勢いをゆっくり弱める
		m_airVelocity *= kAirDrag;
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
			player->ExecJump(kJumpPower);
			m_isTakeoffDone = true;
		}
		return;
	}

	bool isFallingNow = player->GetGravity() > kFallingGravity;

	// --------------------------------------------------
	if (m_jumpPhase == JumpPhaseAir) // 【空中】
	{
		// ジャンプ開始アニメが終わった・落下し始めたら空中ループへ
		if (!m_isLandingAnimStarted && (player->IsAnimEnd() || isFallingNow) && player->GetAnimTime() > 0.0f)
		{
			player->ChangeAnimation(kJumpLoopAnimName, true);
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