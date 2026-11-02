// PlayerStateAttack.cpp
// 攻撃のステート（連続攻撃・アビリティ）
// ※ステートのクラスの宣言は PlayerState.h にまとめてある

#include "PlayerState.h"
#include "Player.h"
#include "PlayerParamManager.h"

#include "../../Effect/EffectPlayer.h"
#include "../../../Scene/SceneManager.h"

#include "PlayerStateConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerStateConst;

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
	if (IsSpinLeapAttack()) return kSpinLeapCancelFrame;		// 回転し終わるまで次へ行かない
	if (m_comboType == 2 && m_comboStep == 4) return 30.0f;		// 落ちる前（空中）のフレームを指定
	if (m_comboType == 2 && m_comboStep == 5) return 999.0f;	// 5段目は最後まで再生

	// 04の4段目：跳び上がって叩きつけるまで（空中で次へ行かないように）最後まで再生
	if (IsJumpSlamAttack()) return 999.0f;

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

HitEffectType PlayerStateComboAttack::GetHitEffectType() const
{
	if (m_comboType == 5)
	{
		switch (m_comboStep)
		{
		case 1:	return HitEffectType::Sword5Slash;	// 2連撃：当たるたびに _Sword5 の斬撃1本
		case 2:	return HitEffectType::Sword5Spin;	// 投げた剣の多段攻撃：当たるたびに回転する斬撃
		case 3:	return HitEffectType::None;			// 瞬間移動の切り刻み：_Sword5 そのまま（決めのエフェクトの方で1回だけ）
		default: break;
		}
	}
	return HitEffectType::Slash;
}

const std::string& PlayerStateComboAttack::GetFinisherEffectName() const
{
	static const std::string kNone = "";

	// 4-3（溜め攻撃）・攻撃タイプ4：タイプ4の斬撃
	if (IsSplitAttack() || m_comboType == 4) return EffectName::ComboSlash4;

	// 攻撃タイプ1・5の4段目（しめの一撃）
	if (m_comboType == 1 && m_comboStep == 4) return EffectName::ComboSlash1_4;
	if (m_comboType == 5 && m_comboStep == 4) return EffectName::ComboSlash5_4;

	// 05の3段目（戻ってきた剣をつかみ、瞬間移動しながら切り刻む）：_Sword5 の4本の斬撃をそのまま
	if (m_comboType == 5 && m_comboStep == 3) return EffectName::ComboSlash5_3;

	return kNone;
}

bool PlayerStateComboAttack::IsTrailFrame(const Player* player, float animTime) const
{
	// 溜め攻撃（4-3）：Start の振り始めから、End で剣を地面に叩きつけるまで、途切れずに出す
	if (IsSplitAttack())
	{
		switch (m_subStep)
		{
		case SubStepStart:	return animTime >= kChargeStartTrailFrame;
		case SubStepLoop:	return true;
		case SubStepEnd:	return !m_isSlamDone;	// 剣が地面に届くまで（届いたフレームも出す）
		default:			break;
		}
	}

	// 04の4段目：剣を抜くところから、跳んで落ちてきて地面に叩きつけるまで
	if (IsJumpSlamAttack())
	{
		float slamFrame = player->GetJumpSlamFrame();
		float endFrame = (slamFrame >= 0.0f) ? slamFrame + kJumpSlamTrailExtraFrame : kAttackHitEndFrame;
		return animTime >= kJumpSlamTrailStartFrame && animTime <= endFrame;
	}

	// 02の3段目：回り始めから着地まで（攻撃判定と同じ）
	if (IsSpinLeapAttack())
	{
		return animTime >= kSpinLeapHitStartFrame && animTime <= kSpinLeapHitEndFrame;
	}

	// 普通の攻撃：攻撃判定のフレームの間
	return animTime >= kAttackHitStartFrame && animTime <= kAttackHitEndFrame;
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

	// 再生速度（05の3段目だけ少しゆっくり。普通の攻撃は、モードで速さを変える）
	bool isSlowAttack = (m_comboType == 5 && m_comboStep == 3);
	float animSpeed = isSlowAttack ? kAttack5_3AnimSpeed : kAttackAnimSpeed;
	if (!m_isAbility) animSpeed *= player->GetModeAttackSpeedRate();
	player->SetAnimationSpeed(animSpeed);

	// アビリティの攻撃中は、当ててもATBゲージがたまらない
	player->SetAbilityAttack(m_isAbility);

	// 新しい振りなので、前の振りで当たった記録を消す（もう一度当たれるようにする）
	player->ResetAttackHits();

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

	float animTime = player->GetAnimTime();

	// 剣の軌跡（攻撃していないフレームはPlayer側で徐々に消える）
	if (IsTrailFrame(player, animTime))
	{
		player->AddSwordTrailPoints();
	}

	// 攻撃タイプ04のLoopは一定時間でEndへ
	// （軌跡を足してから切り替える。先に切り替えると、このフレームに軌跡が足されず、
	//   「振り終わった」とみなされて、End の最初に Loop の軌跡が消えてしまう）
	if (IsSplitAttack() && m_subStep == SubStepLoop)
	{
		m_loopTimer++;
		if (m_loopTimer >= kChargeLoopFrame)
		{
			player->ChangeState(std::make_shared<PlayerStateComboAttack>(4, 3, SubStepEnd, m_nextAttackReserved, m_isAbility));
			return;
		}
	}

	// 溜め攻撃（4-3）の End：剣先が地面に届いたら、次のフレームから軌跡を止める
	if (IsSplitAttack() && m_subStep == SubStepEnd && !m_isSlamDone)
	{
		bool isBlendDone = (animTime >= kSlamBlendFrame);
		bool isOnGround = (player->GetSwordTipHeight(0) <= kSlamGroundHeight);
		if ((isBlendDone && isOnGround) || animTime >= kSlamTrailMaxFrame)
		{
			m_isSlamDone = true;
		}
	}

	// 攻撃判定（04の4段目は、落ちてきて地面に叩きつけるところまで出す。02の3段目は回り始めから着地まで）
	float hitStartFrame = kAttackHitStartFrame;
	float hitEndFrame = kAttackHitEndFrame;
	if (IsJumpSlamAttack() && player->GetJumpSlamFrame() >= 0.0f)
	{
		hitEndFrame = std::max(kAttackHitEndFrame, player->GetJumpSlamFrame() + kJumpSlamHitExtraFrame);
	}
	if (IsSpinLeapAttack())
	{
		hitStartFrame = kSpinLeapHitStartFrame;
		hitEndFrame = kSpinLeapHitEndFrame;
	}
	if (animTime >= hitStartFrame && animTime <= hitEndFrame)
	{
		// 当たったら、攻撃の種類・段数ごとのエフェクトをその位置に出す
		// （ヒットストップは AttackOBB の中で、すべての攻撃に同じルールでかかる）
		// （右と左の剣は別々に当たりを数える。同じ剣は少し間をあければ、もう一度当たる＝多段ヒット）
		float damageRate = m_isAbility ? kAbilityDamageRate : 1.0f;
		player->AttackOBB(player->GetSwordMatrixR(), m_isCritical, GetHitEffectType(), GetFinisherEffectName(), damageRate, 0);
		player->AttackOBB(player->GetSwordMatrixL(), m_isCritical, GetHitEffectType(), GetFinisherEffectName(), damageRate, 1);
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

	// 次の攻撃の予約が無く、少したったら、移動で攻撃の後のすきをキャンセルできる
	// （アビリティ・溜め攻撃の途中・最後まで再生する攻撃は除く）
	float cancelFrame = GetCancelFrame();
	bool canRecoveryCancel = !m_isAbility && !m_nextAttackReserved && !IsSplitAttack()
		&& cancelFrame < kAttackHitEndFrame && animTime >= cancelFrame + kAttackRecoveryFrame;
	if (canRecoveryCancel)
	{
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
