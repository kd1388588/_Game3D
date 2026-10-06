#pragma once

#include"../BaseScene/BaseScene.h"
#include"../../Utility/InputHelper.h"

class UIText;

// ゲームオーバー画面（リトライ / タイトルへ を選択）
class GameOverScene : public BaseScene
{
public :

	GameOverScene()  { Init(); }
	~GameOverScene() {}

private :

	// 選択肢
	enum class Menu
	{
		Retry,	// もう一度ゲームを始める
		Title,	// タイトルへ戻る
		Num
	};

	void Event() override;
	void Init()  override;

	// 選択中の項目を目立たせる
	void UpdateMenuColor();

	// 選択した項目を決定する
	void Decide();

	std::shared_ptr<UIText>	m_menuTexts[static_cast<int>(Menu::Num)];
	Menu					m_select = Menu::Retry;

	InputHelper::KeyTrigger m_upKey{ 'W' };
	InputHelper::KeyTrigger m_downKey{ 'S' };
	InputHelper::KeyTrigger m_upArrowKey{ VK_UP };
	InputHelper::KeyTrigger m_downArrowKey{ VK_DOWN };
	InputHelper::KeyTrigger m_decideKey{ VK_RETURN };
};
