#include "GameClearScene.h"
#include "../SceneManager.h"

#include "../../GameObject/UI/Text/UIText.h"

void GameClearScene::Event()
{
	// Enterでタイトルへ
	if (m_decideKey.Update())
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
	}
}

void GameClearScene::Init()
{
	std::shared_ptr<UIText> title = std::make_shared<UIText>();
	title->SetText("GAME CLEAR", UIText::FontSize::Large);
	title->SetCenterPos({ 0.0f, 120.0f });
	title->SetColor({ 1.0f, 0.85f, 0.2f, 1.0f });
	m_objList.push_back(title);

	std::shared_ptr<UIText> guide = std::make_shared<UIText>();
	guide->SetText("PRESS ENTER", UIText::FontSize::Medium);
	guide->SetCenterPos({ 0.0f, -150.0f });
	guide->SetBlink(true);
	m_objList.push_back(guide);
}
