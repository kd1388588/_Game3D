// PlayerBattle.cpp
// バトルモード・ATBゲージ・ATBコマンドメニュー・アビリティ
// ※Player クラスの関数を、役割ごとにファイルを分けて書いている（クラスの宣言は Player.h）

// Player
#include "Player.h"
#include "../../../Utility/StageSettings.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"
#include "../../../Utility/EffectSettings.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"
#include "../UI/Gage/ATB/ATBGage.h"
#include "../UI/Gage/ATB/ATBCommandMenu.h"

// Enemy
#include "../Enemy/Enemy.h"

// Camera
#include "../../Camera/TPSCamera/TPSCamera.h"

// Effect
#include "../../Effect/EffectPlayer.h"

// Utility
#include "../../../Utility/HitStop.h"
#include "../../../Utility/SoundPlayer.h"
#include "../../../Utility/GameTime.h"

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

#include "PlayerConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerConst;

bool Player::CanUseAbility(int index) const
{
	if (index < 0 || index >= static_cast<int>(AbilitySlot::Num)) return false;
	return m_abilityCT[index] <= 0 && m_atbGage >= kAtbBarMax * kAbilityInfos[index].atbCost;
}

bool Player::UpdateCommandMenu()
{
	// キーの状態は毎フレーム更新する（開いていない時も。押した瞬間を正しく判定するため）
	bool toggle		= m_commandKey.Update();
	bool togglePad	= m_commandPadKey.Update();
	bool up			= m_menuUpKey.Update();
	bool down		= m_menuDownKey.Update();
	bool enter		= m_menuDecideKey.Update();
	bool back		= m_menuBackKey.Update();

	if (!m_isCommandOpen)
	{
		// 倒れている時・アビリティの技を出している途中は、開けない・必殺技も出せない
		if (IsDead() || m_isAbilityAttack) return false;

		// Space ／ パッド A で開く
		if (toggle || togglePad)
		{
			OpenCommandMenu();
			return m_isCommandOpen;
		}

		// 必殺技：1・2 ／ パッド LB ＋ X・Y で、メニューを開かずにすぐ出す（ゲージ・CT はメニューから出した時と同じ）
		for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
		{
			if (ConsumeInput(m_abilityBuffer[i]))
			{
				DecideCommand(i);
				break;
			}
		}
		return m_isCommandOpen;
	}

	// 閉じる：Space・右クリック・R・BackSpace ／ パッド B（パッドの A は、開いている間は決定）
	if (toggle || back || ConsumeInput(m_evadeBuffer))
	{
		CloseCommandMenu();
		return false;
	}

	// 選ぶ：W / S・方向キー ／ パッドの十字キー・左スティック
	constexpr int itemNum = static_cast<int>(AbilitySlot::Num);
	if (up)		m_commandCursor = (m_commandCursor + itemNum - 1) % itemNum;
	if (down)	m_commandCursor = (m_commandCursor + 1) % itemNum;

	// メニューを開いている時に必殺技のキー（1・2 ／ LB ＋ X・Y）を押したら、その技をすぐ決定
	for (int i = 0; i < itemNum; ++i)
	{
		if (ConsumeInput(m_abilityBuffer[i]))
		{
			m_commandCursor = i;
			DecideCommand(i);
			return m_isCommandOpen;
		}
	}

	// 決定：Enter・左クリック・F ／ パッド A・X
	if (enter || ConsumeInput(m_attackBuffer))
	{
		DecideCommand(m_commandCursor);
	}

	return m_isCommandOpen;
}

void Player::OpenCommandMenu()
{
	m_isCommandOpen = true;
	GameTime::SetScale(kCommandTimeScale);
	ClearInputBuffers();

	// 選べる技があれば、最初からそこを選んでおく
	if (!CanUseAbility(m_commandCursor))
	{
		for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
		{
			if (CanUseAbility(i)) { m_commandCursor = i; break; }
		}
	}
}

void Player::CloseCommandMenu()
{
	m_isCommandOpen = false;
	GameTime::SetScale(1.0f);
	ClearInputBuffers();
}

void Player::DecideCommand(int index)
{
	// 使えない技（ゲージが足りない・CT中）なら、メニューを開いたままにする
	int attackType = TryStartAbility(static_cast<AbilitySlot>(index));
	if (attackType <= 0) return;

	// メニューを閉じて時間を戻し、選んだ技を最後の段まで自動で出す
	CloseCommandMenu();
	ChangeState(std::make_shared<PlayerStateComboAttack>(attackType, 1, PlayerStateComboAttack::SubStepNone, true, true));
}

void Player::SetBattleMode(BattleMode mode)
{
	// モードが変わった時は、リムライトで体を光らせて知らせる
	if (mode != m_battleMode)
	{
		m_isLimLight = true;
		m_limPow = 0.0f;
	}

	m_battleMode = mode;

	// エフェクトの色をモードの色にする（エフェクトエディタ。攻撃・モード切り替えのエフェクト）
	EffectSettings::Instance().SetPlayerPunisher(mode == BattleMode::Punisher);

	// 左クリックの攻撃：オペレーターは攻撃01、パニッシャーは攻撃02
	m_currentAttackType = (mode == BattleMode::Punisher) ? kPunisherAttackType : kOperatorAttackType;
	s_currentAttackType = m_currentAttackType;

	// 剣の軌跡の画像をモードの色に変える
	ApplySwordTrailTexture();

	// 画面のモード表示を変える
	if (auto atbGage = m_wpAtbGage.lock())
	{
		atbGage->SetPunisherMode(mode == BattleMode::Punisher);
	}
}

float Player::GetModeMoveRate() const
{
	return (m_battleMode == BattleMode::Punisher) ? PlayerTune().punisherMoveRate : 1.0f;
}

float Player::GetModeAttackSpeedRate() const
{
	return (m_battleMode == BattleMode::Punisher) ? PlayerTune().punisherAttackSpeedRate : PlayerTune().operatorAttackSpeedRate;
}

void Player::UpdateModeSwitch()
{
	// Eキーが押されたら、オペレーター ⇔ パニッシャー を切り替える
	// （キーの状態は毎フレーム更新する。倒れている時は切り替えない）
	if (!m_modeKey.Update() || IsDead()) return;

	// 攻撃05を使った後は、しばらくオペレーターに固定（切り替えられない）
	if (m_modeLockTimer > 0)
	{
		KdDebugGUI::Instance().AddLog("[System] モード固定中（あと %.1f 秒）\n", m_modeLockTimer / 60.0f);
		return;
	}

	// 切り替えた後は、しばらく切り替えられない（クールタイム）
	if (m_modeSwitchCT > 0)
	{
		KdDebugGUI::Instance().AddLog("[System] モード切り替えのクールタイム中（あと %.1f 秒）\n", m_modeSwitchCT / 60.0f);
		return;
	}
	m_modeSwitchCT = kModeSwitchCoolFrame;

	bool toPunisher = (m_battleMode == BattleMode::Operator);
	SetBattleMode(toPunisher ? BattleMode::Punisher : BattleMode::Operator);

	// 切り替えた合図のエフェクト（足元）。白く作ってあり、エフェクトエディタ（EffectSettings）のモードの色で青・オレンジになる
	EffectPlayer::Play(EffectName::ModeChange, m_pos, 1.0f);

	KdDebugGUI::Instance().AddLog
	(
		"[System] モード切り替え: %s\n",
		toPunisher ? "PUNISHER" : "OPERATOR"
	);
}

void Player::AddAtbGage(float val)
{
	m_atbGage = std::clamp(m_atbGage + val, 0.0f, kAtbBarMax * kAtbBarNum);
}

void Player::GainAtbByEvade()
{
	AddAtbGage(PlayerTune().atbEvadeGain);
}

int Player::TryStartAbility(AbilitySlot slot)
{
	int index = static_cast<int>(slot);
	if (index < 0 || index >= static_cast<int>(AbilitySlot::Num)) return 0;
	const AbilityInfo& info = kAbilityInfos[index];

	// CT中は使えない
	if (m_abilityCT[index] > 0)
	{
		KdDebugGUI::Instance().AddLog("[System] %s はCT中（あと %.1f 秒）\n", info.name, m_abilityCT[index] / 60.0f);
		return 0;
	}

	// ATBゲージが足りなければ使えない
	float cost = kAtbBarMax * info.atbCost;
	if (m_atbGage < cost)
	{
		KdDebugGUI::Instance().AddLog("[System] %s にはATBゲージが %d 本必要です\n", info.name, info.atbCost);
		return 0;
	}

	// ゲージを使い、CTを始める
	m_atbGage -= cost;
	m_abilityCT[index] = info.coolTime;

	// 攻撃05の後は、しばらくオペレーターに固定する
	if (info.attackType == kModeLockAttackType)
	{
		SetBattleMode(BattleMode::Operator);
		m_modeLockTimer = kModeLockFrame;
	}

	EffectPlayer::Play(EffectName::AbilityStart, m_pos, 1.0f);
	KdDebugGUI::Instance().AddLog("[System] アビリティ発動: %s\n", info.name);
	return info.attackType;
}

void Player::UpdateAbilityTimers()
{
	for (int& ct : m_abilityCT)
	{
		if (ct > 0) ct--;
	}
	if (m_modeLockTimer > 0) m_modeLockTimer--;
	if (m_modeSwitchCT > 0) m_modeSwitchCT--;

	// ガードの耐久値
	if (m_guardBreakTimer > 0)
	{
		// 崩された後：ガードできない間も、耐久値は 0 から少しずつ回復する
		// （一気に満タンに戻さず、ゲージがたまっていくのが見えるようにする）
		m_guardBreakTimer--;
		m_guardDurability = std::min(m_guardDurability + PlayerTune().guardRegenSpeed, PlayerTune().guardDurabilityMax);
	}
	else if (m_guardRegenDelay > 0)
	{
		m_guardRegenDelay--;
	}
	else
	{
		m_guardDurability = std::min(m_guardDurability + PlayerTune().guardRegenSpeed, PlayerTune().guardDurabilityMax);
	}
}

void Player::UpdateAtb()
{
	// UIへ伝える
	if (auto atbGage = m_wpAtbGage.lock())
	{
		// 何本分たまっているか（例：1.5 なら 1本目が満タン、2本目が半分）
		atbGage->SetAtbValue(m_atbGage / kAtbBarMax);

		// アビリティ：使えるか・CTの残りの割合
		for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
		{
			const AbilityInfo& info = kAbilityInfos[i];
			bool canUse = (m_abilityCT[i] <= 0) && (m_atbGage >= kAtbBarMax * info.atbCost);
			float ctRatio = static_cast<float>(m_abilityCT[i]) / static_cast<float>(info.coolTime);
			atbGage->SetAbilityState(i, canUse, ctRatio, info.atbCost);
		}

		// モードを切り替えられない残りの割合（攻撃05の後の固定と、切り替えのクールタイムの長い方）・ガード中か
		float lockRatio = static_cast<float>(m_modeLockTimer) / static_cast<float>(kModeLockFrame);
		float coolRatio = static_cast<float>(m_modeSwitchCT) / static_cast<float>(kModeSwitchCoolFrame);
		atbGage->SetModeLockRatio(std::max(lockRatio, coolRatio));
		atbGage->SetGuarding(m_isGuarding);
		atbGage->SetGuardState(m_guardDurability / PlayerTune().guardDurabilityMax, m_guardBreakTimer > 0);

		// ATBコマンドメニュー
		if (auto commandMenu = m_wpCommandMenu.lock())
		{
			commandMenu->SetOpen(m_isCommandOpen);
			commandMenu->SetCursor(m_commandCursor);
			for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
			{
				const AbilityInfo& info = kAbilityInfos[i];
				float ctRatio = static_cast<float>(m_abilityCT[i]) / static_cast<float>(info.coolTime);
				commandMenu->SetItemState(i, CanUseAbility(i), ctRatio, info.atbCost);
			}
		}
	}
}
