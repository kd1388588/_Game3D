#pragma once

#include "../main.h"

#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

// キー入力・パッド（Xbox コントローラー）入力の補助関数・クラス
//
// ■ 操作の割り当て（パッド ／ キーボード・マウス）
//   移動　　　　　　：左スティック ／ WASD
//   カメラ　　　　　：右スティック ／ マウス
//   攻撃　　　　　　：X ／ F・左クリック
//   回避　　　　　　：B ／ R・右クリック
//   モード切り替え　：Y ／ E・ホイールクリック
//   ATBコマンド　　 ：A ／ Space
//   必殺技1・2　　　：LB ＋ X・LB ＋ Y ／ 1・2
//   ガード・ダッシュ：RB ／ Ctrl（止まっていればガード、走っていればダッシュ）
//   メニュー　　　　：十字キー・左スティック、A で決定、B で戻る ／ W・S・方向キー、Enter・左クリックで決定、BackSpace・右クリックで戻る
namespace InputHelper
{
	// 指定キーが押されているか
	inline bool IsKeyDown(int vKey)
	{
		return (GetAsyncKeyState(vKey) & 0x8000) != 0;
	}

	// ============================================================
	// パッド（XInput。Xbox コントローラー）
	// ・1台目（0番）だけを使う
	// ・GamePad::Update() を毎フレームの最初に1回だけ呼ぶ（main.cpp）
	// ============================================================
	enum class PadButton
	{
		None,
		A, B, X, Y,
		LB, RB,
		Start, Back,
		DPadUp, DPadDown, DPadLeft, DPadRight,
		LStick, RStick,			// スティックの押し込み
		LT, RT,					// トリガー（半分より深く引いたら押したとみなす）
		LStickUp, LStickDown,	// 左スティックを上下に倒した（メニューの上下に使う）
	};

	class GamePad
	{
	public:
		static void Update()
		{
			// つながっていない時は、1秒に1回だけ調べる（つながっていないパッドを毎フレーム調べると重くなるため）
			if (!s_isConnected && (s_retryTimer++ % kRetryInterval) != 0) return;

			XINPUT_STATE state = {};
			s_isConnected = (XInputGetState(0, &state) == ERROR_SUCCESS);
			if (!s_isConnected) state = {};
			s_state = state;

			s_leftStick = NormalizeStick(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			s_rightStick = NormalizeStick(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
		}

		static bool IsConnected() { return s_isConnected; }

		static bool IsDown(PadButton button)
		{
			if (!s_isConnected) return false;
			const XINPUT_GAMEPAD& pad = s_state.Gamepad;
			switch (button)
			{
			case PadButton::A:			return (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
			case PadButton::B:			return (pad.wButtons & XINPUT_GAMEPAD_B) != 0;
			case PadButton::X:			return (pad.wButtons & XINPUT_GAMEPAD_X) != 0;
			case PadButton::Y:			return (pad.wButtons & XINPUT_GAMEPAD_Y) != 0;
			case PadButton::LB:			return (pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
			case PadButton::RB:			return (pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
			case PadButton::Start:		return (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
			case PadButton::Back:		return (pad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
			case PadButton::DPadUp:		return (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
			case PadButton::DPadDown:	return (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
			case PadButton::DPadLeft:	return (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
			case PadButton::DPadRight:	return (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
			case PadButton::LStick:		return (pad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
			case PadButton::RStick:		return (pad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
			case PadButton::LT:			return pad.bLeftTrigger > kTriggerThreshold;
			case PadButton::RT:			return pad.bRightTrigger > kTriggerThreshold;
			case PadButton::LStickUp:	return s_leftStick.y > kStickButtonThreshold;
			case PadButton::LStickDown:	return s_leftStick.y < -kStickButtonThreshold;
			default:					return false;
			}
		}

		// スティックの倒し具合（-1～1。遊び（デッドゾーン）の中は 0）
		static const Math::Vector2& GetLeftStick() { return s_leftStick; }
		static const Math::Vector2& GetRightStick() { return s_rightStick; }

	private:
		static Math::Vector2 NormalizeStick(SHORT x, SHORT y, int deadZone)
		{
			Math::Vector2 v = { static_cast<float>(x), static_cast<float>(y) };
			float length = v.Length();
			if (length <= static_cast<float>(deadZone)) return Math::Vector2::Zero;

			// 遊びの外側を 0～1 に広げ直す（遊びのすぐ外で急に動き出さないように）
			float rate = (std::min(length, 32767.0f) - deadZone) / (32767.0f - deadZone);
			return v / length * rate;
		}

		static constexpr BYTE	kTriggerThreshold		= 128;
		static constexpr int	kRetryInterval			= 60;
		inline static int		s_retryTimer			= 0;
		static constexpr float	kStickButtonThreshold	= 0.5f;

		inline static XINPUT_STATE	s_state = {};
		inline static bool			s_isConnected = false;
		inline static Math::Vector2	s_leftStick = Math::Vector2::Zero;
		inline static Math::Vector2	s_rightStick = Math::Vector2::Zero;
	};

	// ============================================================
	// 1つの操作に割り当てたキー・パッドのボタンのどれかが押されているか
	// padModifier：パッドのボタンは、このボタンを押している時（required = true）、
	//              または押していない時（required = false）だけ数える（LB ＋ X の必殺技と、X の攻撃を分けるため）
	// keyModifier：キーも同じように、このキー（Shift など）を押している・押していない時だけ数える（デバッグキー用）
	// ============================================================
	struct InputBinding
	{
		std::vector<int>		keys;
		std::vector<PadButton>	pads;
		PadButton				padModifier = PadButton::None;
		bool					padModifierRequired = false;
		int						keyModifier = 0;
		bool					keyModifierRequired = false;

		bool IsDown() const
		{
			bool isKeyModifierOk = (keyModifier == 0) || (IsKeyDown(keyModifier) == keyModifierRequired);
			if (isKeyModifierOk)
			{
				for (int key : keys)
				{
					if (IsKeyDown(key)) return true;
				}
			}
			bool isModifierOk = (padModifier == PadButton::None) || (GamePad::IsDown(padModifier) == padModifierRequired);
			if (!isModifierOk) return false;
			for (PadButton pad : pads)
			{
				if (GamePad::IsDown(pad)) return true;
			}
			return false;
		}
	};

	// キー・ボタンを「押した瞬間」だけ true を返す
	class KeyTrigger
	{
	public:
		// 作った時点で押されているキーは「押した瞬間」とみなさない
		// （シーン切り替え直後に、前のシーンで押したキーで反応しないように）
		explicit KeyTrigger(int vKey) : KeyTrigger(InputBinding{ { vKey }, {} }) {}
		explicit KeyTrigger(const InputBinding& binding) : m_binding(binding), m_isPrevDown(binding.IsDown()) {}

		// 毎フレーム1回だけ呼ぶこと（前フレームの状態を内部で更新する）
		bool Update()
		{
			bool isDown = m_binding.IsDown();
			bool isTrigger = isDown && !m_isPrevDown;
			m_isPrevDown = isDown;
			return isTrigger;
		}

		bool IsDown() const { return m_binding.IsDown(); }

	private:
		InputBinding	m_binding;
		bool			m_isPrevDown = false;
	};

	// ============================================================
	// ゲームで使う操作の割り当て（変える時はここだけを直す）
	// ============================================================
	namespace Bind
	{
		inline const InputBinding Attack		= { { 'F', VK_LBUTTON },	{ PadButton::X },	PadButton::LB, false };
		inline const InputBinding Evade			= { { 'R', VK_RBUTTON },	{ PadButton::B } };
		inline const InputBinding ModeChange	= { { 'E', VK_MBUTTON },	{ PadButton::Y },	PadButton::LB, false };
		inline const InputBinding CommandKey	= { { VK_SPACE },			{} };				// キーボード：開く・閉じる
		inline const InputBinding CommandPad	= { {},						{ PadButton::A } };	// パッド：開く（メニュー中は決定）
		inline const InputBinding Special1		= { { '1' },				{ PadButton::X },	PadButton::LB, true };
		inline const InputBinding Special2		= { { '2' },				{ PadButton::Y },	PadButton::LB, true };
		inline const InputBinding GuardDash		= { { VK_CONTROL },			{ PadButton::RB } };

		// メニュー（ATBコマンド・タイトル・リザルト）
		inline const InputBinding MenuUp		= { { 'W', VK_UP },			{ PadButton::DPadUp, PadButton::LStickUp } };
		inline const InputBinding MenuDown		= { { 'S', VK_DOWN },		{ PadButton::DPadDown, PadButton::LStickDown } };
		inline const InputBinding MenuDecide	= { { VK_RETURN },			{ PadButton::A } };
		inline const InputBinding MenuBack		= { { VK_BACK },			{ PadButton::B } };

		// デバッグ（キーボードだけ。F12 でデバッグ表示を出している間だけ効く）
		inline const InputBinding DebugAtb			= { { VK_F9 },	{}, PadButton::None, false, VK_SHIFT, false };
		inline const InputBinding DebugHpFull		= { { VK_F10 },	{}, PadButton::None, false, VK_SHIFT, false };
		inline const InputBinding DebugHpOne		= { { VK_F11 },	{}, PadButton::None, false, VK_SHIFT, false };
		inline const InputBinding DebugSpawn		= { { VK_F9 },	{}, PadButton::None, false, VK_SHIFT, true };
		inline const InputBinding DebugInvincible	= { { VK_F10 },	{}, PadButton::None, false, VK_SHIFT, true };
		inline const InputBinding DebugWarpBoss		= { { VK_F11 },	{}, PadButton::None, false, VK_SHIFT, true };
		inline const InputBinding DebugToTitle		= { { VK_F12 },	{}, PadButton::None, false, VK_SHIFT, true };
	}

	// マウスの位置を、UIの座標（画面の中央が原点、上が＋Y。1280x720）で取得する
	// （ウィンドウの大きさが変わっても合うように、割合で計算する）
	inline bool GetMouseUIPos(Math::Vector2& outPos)
	{
		constexpr float kScreenWidth	= 1280.0f;
		constexpr float kScreenHeight	= 720.0f;

		POINT point;
		if (!GetCursorPos(&point)) return false;

		HWND hwnd = Application::Instance().GetWindowHandle();
		if (!ScreenToClient(hwnd, &point)) return false;

		RECT rect;
		if (!GetClientRect(hwnd, &rect)) return false;
		float width = static_cast<float>(rect.right - rect.left);
		float height = static_cast<float>(rect.bottom - rect.top);
		if (width <= 0.0f || height <= 0.0f) return false;

		outPos.x = (static_cast<float>(point.x) / width - 0.5f) * kScreenWidth;
		outPos.y = (0.5f - static_cast<float>(point.y) / height) * kScreenHeight;
		return true;
	}
}
