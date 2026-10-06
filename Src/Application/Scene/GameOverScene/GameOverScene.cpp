#include "GameOverScene.h"
#include "../SceneManager.h"

#include "../../GameObject/UI/Text/UIText.h"

namespace
{
	const Math::Color kSelectColor		= { 1.0f, 0.85f, 0.2f, 1.0f };	// 選択中の項目の色
	const Math::Color kUnselectColor	= { 0.5f, 0.5f, 0.5f, 1.0f };	// 選択していない項目の色
	constexpr float kMenuTopY			= -80.0f;	// 一番上の項目の高さ
	constexpr float kMenuSpaceY			= 70.0f;	// 項目の間隔
}

void GameOverScene::Event()
{
	// 入力は毎フレーム全て更新する（押した瞬間の判定のため）
	bool isUp = m_upKey.Update() | m_upArrowKey.Update();
	bool isDown = m_downKey.Update() | m_downArrowKey.Update();
	bool isDecide = m_decideKey.Update();

	const int menuNum = static_cast<int>(Menu::Num);
	int select = static_cast<int>(m_select);

	if (isUp)	select = (select + menuNum - 1) % menuNum;
	if (isDown)	select = (select + 1) % menuNum;

	if (select != static_cast<int>(m_select))
	{
		m_select = static_cast<Menu>(select);
		UpdateMenuColor();
	}

	if (isDecide)
	{
		Decide();
	}
}

void GameOverScene::Init()
{
	std::shared_ptr<UIText> title = std::make_shared<UIText>();
	title->SetText("GAME OVER", UIText::FontSize::Large);
	title->SetCenterPos({ 0.0f, 150.0f });
	title->SetColor({ 0.9f, 0.15f, 0.15f, 1.0f });
	m_objList.push_back(title);

	// 選択肢（Menuの並び順と合わせる）
	const std::string menuNames[] = { "RETRY", "TITLE" };
	for (int i = 0; i < static_cast<int>(Menu::Num); ++i)
	{
		m_menuTexts[i] = std::make_shared<UIText>();
		m_menuTexts[i]->SetText(menuNames[i], UIText::FontSize::Medium);
		m_menuTexts[i]->SetCenterPos({ 0.0f, kMenuTopY - kMenuSpaceY * i });
		m_objList.push_back(m_menuTexts[i]);
	}

	UpdateMenuColor();
}

void GameOverScene::UpdateMenuColor()
{
	for (int i = 0; i < static_cast<int>(Menu::Num); ++i)
	{
		bool isSelected = (i == static_cast<int>(m_select));
		m_menuTexts[i]->SetColor(isSelected ? kSelectColor : kUnselectColor);
		m_menuTexts[i]->SetBlink(isSelected);
	}
}

void GameOverScene::Decide()
{
	switch (m_select)
	{
	case Menu::Retry:
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Game);
		break;
	case Menu::Title:
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
		break;
	default:
		break;
	}
}
