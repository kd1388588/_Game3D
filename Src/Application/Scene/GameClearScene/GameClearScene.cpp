#include "GameClearScene.h"
#include "../SceneManager.h"

#include "../../GameObject/UI/Text/UIText.h"
#include "../../GameObject/UI/Background/UIBackground.h"

void GameClearScene::Event()
{
	// 左クリック・Enterでタイトルへ（入力は毎フレーム両方とも更新する）
	bool isDecide = m_decideKey.Update();
	bool isClick = m_clickKey.Update();
	if (isDecide || isClick)
	{
		SceneManager::Instance().SetNextScene(SceneManager::SceneType::Title);
	}
}

void GameClearScene::Init()
{
	// 背景（一番最初に追加して、一番奥に描く）
	std::shared_ptr<UIBackground> background = std::make_shared<UIBackground>();
	background->Load("Asset/Textures/UI/Result/GameClear.png");
	m_objList.push_back(background);

	std::shared_ptr<UIText> title = std::make_shared<UIText>();
	title->SetText("GAME CLEAR", UIText::FontSize::Large);
	title->SetCenterPos({ 0.0f, 120.0f });
	title->SetColor({ 1.0f, 0.85f, 0.2f, 1.0f });
	m_objList.push_back(title);

	std::shared_ptr<UIText> guide = std::make_shared<UIText>();
	guide->SetText("CLICK TO TITLE", UIText::FontSize::Medium);
	guide->SetCenterPos({ 0.0f, -150.0f });
	guide->SetBlink(true);
	m_objList.push_back(guide);
}
