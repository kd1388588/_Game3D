#pragma once
#include "../Base/BaseGage.h"

class UIText;

// ATBコマンドメニュー（FF7リメイク風）
// ・開いている間は時間がゆっくりになり、ATBゲージを使う技（攻撃04・05）を選ぶ
// ・選んだ技は、最後の段まで自動で出る（Player 側で処理）
class ATBCommandMenu : public BaseGage
{
public:
	ATBCommandMenu() {}
	~ATBCommandMenu() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	// 開く・閉じる
	void SetOpen(bool isOpen) { m_isOpen = isOpen; }

	// 選んでいる項目
	void SetCursor(int index) { m_cursor = index; }

	// 項目の状態（canUse：今選べるか、ctRatio：CTの残りの割合、cost：使うATBゲージの本数）
	void SetItemState(int index, bool canUse, float ctRatio, int cost);

private:

	struct ItemState
	{
		bool	canUse = false;
		float	ctRatio = 0.0f;
		int		cost = 1;
	};

	bool									m_isOpen = false;
	float									m_openRate = 0.0f;	// 開き具合（0～1。なめらかに開く・閉じる）
	int										m_cursor = 0;
	int										m_frame = 0;
	std::vector<ItemState>					m_items;

	std::shared_ptr<UIText>					m_titleText = nullptr;
	std::shared_ptr<UIText>					m_helpText = nullptr;
	std::vector<std::shared_ptr<UIText>>	m_nameTexts;		// 技の名前
	std::vector<std::shared_ptr<UIText>>	m_costTexts;		// 使うゲージの本数
};
