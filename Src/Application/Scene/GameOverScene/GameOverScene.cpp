#include "GameOverScene.h"
#include "../../Utility/SoundPlayer.h"
#include "../SceneManager.h"

#include "../../GameObject/UI/Text/UIText.h"
#include "../../GameObject/UI/Background/UIBackground.h"

namespace
{
	const Math::Color kSelectColor		= { 1.0f, 0.85f, 0.3f, 1.0f };	// 選択中の項目の色
	const Math::Color kUnselectColor	= { 0.8f, 0.8f, 0.8f, 1.0f };	// 選択していない項目の色（暗い背景でも読める明るさ）
	constexpr float kSelectScale		= 1.15f;	// 選択中の項目を少し大きくする
	constexpr float kMenuTopY			= -80.0f;	// 一番上の項目の高さ
	constexpr float kMenuSpaceY			= 70.0f;	// 項目の間隔
	constexpr float kMenuHitMarginX		= 40.0f;	// マウスの当たり判定を、文字より少し広くする
	constexpr float kMenuHitMarginY		= 12.0f;
}

void GameOverScene::Event()
{
	// 入力は毎フレーム全て更新する（押した瞬間の判定のため）
	bool isUp = m_upKey.Update() | m_upArrowKey.Update();
	bool isDown = m_downKey.Update() | m_downArrowKey.Update();
	bool isDecide = m_decideKey.Update();
	bool isClick = m_clickKey.Update();

	const int menuNum = static_cast<int>(Menu::Num);
	int select = static_cast<int>(m_select);

	if (isUp)	select = (select + menuNum - 1) % menuNum;
	if (isDown)	select = (select + 1) % menuNum;

	// マウス：動かした時に乗っている項目を選ぶ。クリックで決定
	int hovered = -1;
	Math::Vector2 mousePos;
	if (InputHelper::GetMouseUIPos(mousePos))
	{
		hovered = GetHoveredMenu(mousePos);
		bool isMouseMoved = (mousePos - m_prevMousePos).LengthSquared() > 1.0f;
		if (isMouseMoved && hovered >= 0) select = hovered;
		m_prevMousePos = mousePos;
	}
	if (isClick && hovered >= 0)
	{
		select = hovered;
		isDecide = true;
	}

	if (select != static_cast<int>(m_select))
	{
		m_select = static_cast<Menu>(select);
		UpdateMenuColor();
		SoundPlayer::PlaySE(SoundName::MenuSelect);
	}

	if (isDecide)
	{
		Decide();
	}
}

void GameOverScene::Init()
{
	// BGM
	SoundPlayer::PlayBGM(SoundName::BgmGameOver, false);

	// 背景（一番最初に追加して、一番奥に描く）
	std::shared_ptr<UIBackground> background = std::make_shared<UIBackground>();
	background->Load("Asset/Textures/UI/Result/GameOver.png");
	m_objList.push_back(background);

	std::shared_ptr<UIText> title = std::make_shared<UIText>();
	title->SetText("GAME OVER", UIText::FontSize::Large);
	title->SetCenterPos({ 0.0f, 150.0f });
	title->SetColor({ 1.0f, 0.3f, 0.25f, 1.0f });
	m_objList.push_back(title);

	// 選択肢（Menuの並び順と合わせる）
	const std::string menuNames[] = { "RETRY", "TITLE" };
	for (int i = 0; i < static_cast<int>(Menu::Num); ++i)
	{
		m_menuTexts[i] = std::make_shared<UIText>();
		m_menuTexts[i]->SetText(menuNames[i], UIText::FontSize::LabelMedium);
		m_menuTexts[i]->SetCenterPos({ 0.0f, kMenuTopY - kMenuSpaceY * i });
		m_objList.push_back(m_menuTexts[i]);
	}

	UpdateMenuColor();
}

int GameOverScene::GetHoveredMenu(const Math::Vector2& mousePos) const
{
	for (int i = 0; i < static_cast<int>(Menu::Num); ++i)
	{
		if (!m_menuTexts[i]) continue;

		Math::Vector2 center = { 0.0f, kMenuTopY - kMenuSpaceY * i };
		Math::Vector2 half = m_menuTexts[i]->GetSize() * 0.5f;
		if (std::abs(mousePos.x - center.x) <= half.x + kMenuHitMarginX &&
			std::abs(mousePos.y - center.y) <= half.y + kMenuHitMarginY)
		{
			return i;
		}
	}
	return -1;
}

void GameOverScene::UpdateMenuColor()
{
	for (int i = 0; i < static_cast<int>(Menu::Num); ++i)
	{
		bool isSelected = (i == static_cast<int>(m_select));
		m_menuTexts[i]->SetColor(isSelected ? kSelectColor : kUnselectColor);
		// 点滅させると消えている間に何が選ばれているか分からないので、色と大きさで目立たせる
		m_menuTexts[i]->SetScale(isSelected ? kSelectScale : 1.0f);
	}
}

void GameOverScene::Decide()
{
	SoundPlayer::PlaySE(SoundName::MenuDecide);

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
