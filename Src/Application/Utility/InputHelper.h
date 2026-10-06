#pragma once

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
}
