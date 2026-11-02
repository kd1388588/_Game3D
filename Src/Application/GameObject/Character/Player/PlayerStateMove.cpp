// PlayerStateMove.cpp
// 移動のステート（走る・ダッシュ・急ブレーキ・回避・ジャンプ）
// ※ステートのクラスの宣言は PlayerState.h にまとめてある

#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../Scene/SceneManager.h"

#include "PlayerStateConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerStateConst;

void PlayerStateRun::ChangeState(Player* player)
{
	m_dashPhase = DashPhase::None;
	player->ChangeAnimation(kRunAnimName, true, false, kRunBlendFrame);
	player->SetAnimationSpeed(1.0f);

	// 急ブレーキの途中で走り出した時は、剣を納めたまま全力疾走から始める
	if (m_startInDash && player->HasAnimation(kDashAnimName))
	{
		ChangeDashPhase(player, DashPhase::Dash);
	}

	// 走りのアニメーションの動きで前へ進む（足の動きと移動の速さが一致するので、足が地面を滑らない）
	UseRootMotion(player);
}

void PlayerStateRun::ChangeDashPhase(Player* player, DashPhase phase)
{
	m_dashPhase = phase;
	switch (phase)
	{
	case DashPhase::None:		player->ChangeAnimation(kRunAnimName, true, false, kDashBlendFrame);			break;
	case DashPhase::Sheathe:	player->ChangeAnimation(kRunSheatheAnimName, false, true, kDashBlendFrame);	break;
	case DashPhase::Dash:		player->ChangeAnimation(kDashAnimName, true, false, kDashBlendFrame);			break;
	}
}

void PlayerStateRun::Update(Player* player)
{
	if (TryCommonTransition(player)) return;

	// ダッシュのアニメーションの切り替え（アニメーションが無い時は、普段の走りのままダッシュする）
	// 一度ダッシュを始めたら、Ctrl ／ RB を離しても移動キーを離すまで全力で走り続ける
	if (player->HasAnimation(kDashAnimName))
	{
		switch (m_dashPhase)
		{
		case DashPhase::None:
			if (player->IsGuardDashDown()) ChangeDashPhase(player, player->HasAnimation(kRunSheatheAnimName) ? DashPhase::Sheathe : DashPhase::Dash);
			break;
		case DashPhase::Sheathe:
			if (player->IsAnimEnd()) ChangeDashPhase(player, DashPhase::Dash);
			break;
		case DashPhase::Dash:
			break;
		}
	}
	bool isDash = (m_dashPhase != DashPhase::None) || player->IsGuardDashDown();

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
		speedRate *= player->GetModeMoveRate();								// パニッシャーの時は遅く
		if (isDash) speedRate *= kDashSpeedRate;							// ガード・ダッシュのキー（Ctrl ／ RB）を押している間はダッシュ
		// 全力疾走のアニメーションは、それ自体が普段の走りより速いので、その分を割り引く
		if (m_dashPhase == DashPhase::Dash) speedRate /= player->GetDashAnimSpeedRatio();

		float animSpeed = 1.0f + (speedRate - 1.0f) * kRunAnimSpeedShare;
		player->SetAnimationSpeed(animSpeed);
		player->SetRootMotionScale(speedRate / animSpeed);
	}

	// アニメーションの動きで進む時は、入力方向へ向きを変えるだけ
	if (!player->MoveProcess(!isAnimMotion))
	{
		// 剣を納めて走っている時は、急ブレーキ → 抜刀 してから止まる
		bool isSheathed = (m_dashPhase != DashPhase::None);
		if (isSheathed && player->HasAnimation(kDashBrakeAnimName))
		{
			player->ChangeState(std::make_shared<PlayerStateDashStop>());
		}
		else
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
	}
}

float PlayerStateRun::GetFootstepVolume() const
{
	return (m_dashPhase == DashPhase::None) ? kRunFootstepVolume : kDashFootstepVolume;
}

float PlayerStateDashStop::GetFootstepVolume() const
{
	return kDashFootstepVolume;
}

void PlayerStateDashStop::ChangeState(Player* player)
{
	// 急ブレーキの滑る動きは、アニメーションの動きどおりに動かす
	UseRootMotion(player);
	ChangePhase(player, Phase::Brake);
}

void PlayerStateDashStop::ChangePhase(Player* player, Phase phase)
{
	m_phase = phase;
	switch (phase)
	{
	case Phase::Brake:
		player->ChangeAnimation(kDashBrakeAnimName, false, true, kDashBlendFrame);
		player->SetAnimationSpeed(1.0f);
		player->SetRootMotionScale(1.0f);
		break;
	case Phase::Draw:
		player->ChangeAnimation(kIdleDrawAnimName, false, true, kDashBlendFrame);
		player->SetAnimationSpeed(kIdleDrawAnimSpeed);
		break;
	}
}

void PlayerStateDashStop::Update(Player* player)
{
	// 回避・ジャンプ・攻撃はいつでもできる（攻撃は剣を抜きながら出す）
	if (TryCommonTransition(player)) return;

	switch (m_phase)
	{
	case Phase::Brake:
		// ブレーキの途中でまた走り出したら、剣を納めたまま全力疾走に戻る
		if (player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>(true));
			return;
		}
		if (player->IsAnimEnd())
		{
			// 抜刀のアニメーションが無い時は、そのまま待機へ
			if (player->HasAnimation(kIdleDrawAnimName))	ChangePhase(player, Phase::Draw);
			else											player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
		break;

	case Phase::Draw:
	{
		// 抜刀がある程度進んだら、移動で普段の走りに移れる
		float animLength = player->GetAnimLength();
		bool canMove = (animLength > 0.0f) && (player->GetAnimTime() >= animLength * kIdleDrawCancelRate);
		if (canMove && player->CheckMoveInput())
		{
			player->ChangeState(std::make_shared<PlayerStateRun>());
			return;
		}
		if (player->IsAnimEnd())
		{
			player->ChangeState(std::make_shared<PlayerStateIdle>());
		}
		break;
	}
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

	// 回避は途中でキャンセルできない（攻撃・ジャンプ・回避は受け付けない）
	// 起き上がりの終わり際で移動キーが押されていれば、そのまま走りへなめらかにつなぐ
	float animLength = player->GetAnimLength();
	bool canReturnToMove = (animLength > 0.0f) && (player->GetAnimTime() >= animLength * kEvadeReturnMoveRate);
	if (canReturnToMove && player->CheckMoveInput())
	{
		player->ChangeState(std::make_shared<PlayerStateRun>());
		return;
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
