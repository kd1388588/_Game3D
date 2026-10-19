#pragma once

#include "../main.h"

// キー入力の補助関数・クラス
namespace InputHelper
{
	// 指定キーが押されているか
	inline bool IsKeyDown(int vKey)
	{
		return (GetAsyncKeyState(vKey) & 0x8000) != 0;
	}

	// キーを「押した瞬間」だけ true を返す
	class KeyTrigger
	{
	public:
		// 作った時点で押されているキーは「押した瞬間」とみなさない
		// （シーン切り替え直後に、前のシーンで押したキーで反応しないように）
		explicit KeyTrigger(int vKey) : m_vKey(vKey), m_isPrevDown(IsKeyDown(vKey)) {}

		// 毎フレーム1回だけ呼ぶこと（前フレームの状態を内部で更新する）
		bool Update()
		{
			bool isDown = IsKeyDown(m_vKey);
			bool isTrigger = isDown && !m_isPrevDown;
			m_isPrevDown = isDown;
			return isTrigger;
		}

	private:
		int		m_vKey = 0;
		bool	m_isPrevDown = false;
	};

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
