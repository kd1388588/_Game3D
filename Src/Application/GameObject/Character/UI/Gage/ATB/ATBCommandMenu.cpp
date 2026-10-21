#include "ATBCommandMenu.h"
#include "../../../Player/Player.h"
#include "../../../../UI/Text/UIText.h"

namespace
{
	// 選べる技（並びは AbilitySlot と同じ）
	const char* const	kItemNames[]		= { "攻撃05", "攻撃04" };	// 上から（Player.cpp の kAbilityInfos と同じ順番）

	// 位置・大きさ（画面の右下）
	constexpr int		kPanelCenterX		= 420;
	constexpr int		kPanelCenterY		= -205;
	constexpr int		kPanelHalfW			= 190;
	constexpr int		kPanelHalfH			= 95;
	constexpr float		kTitleY				= -132.0f;
	constexpr float		kFirstItemY			= -175.0f;
	constexpr float		kItemSpacing		= 42.0f;
	constexpr float		kHelpY				= -277.0f;
	constexpr float		kNameLeftX			= 270.0f;	// 技の名前の左端
	constexpr float		kCostRightX			= 590.0f;	// 使うゲージの本数の右端
	constexpr int		kItemHalfH			= 17;		// 選んでいる項目の背景の高さ（半分）
	constexpr int		kCTBarHeight		= 3;		// CTの残りのバーの太さ

	// 色
	const Math::Color	kDimColor			= { 0.0f, 0.0f, 0.05f, 0.35f };	// 画面全体を少し暗くする
	const Math::Color	kPanelColor			= { 0.03f, 0.05f, 0.12f, 0.85f };
	const Math::Color	kPanelFrameColor	= { 0.6f, 0.75f, 0.95f, 0.9f };
	const Math::Color	kCursorColor		= { 1.0f, 0.85f, 0.3f, 0.3f };		// 選んでいる項目の背景
	const Math::Color	kCursorMarkColor	= { 1.0f, 0.85f, 0.3f, 1.0f };		// 選んでいる項目の左の三角
	const Math::Color	kEnableColor		= { 1.0f, 1.0f, 1.0f, 1.0f };		// 選べる項目の文字
	const Math::Color	kDisableColor		= { 0.45f, 0.45f, 0.5f, 1.0f };		// 選べない項目の文字
	const Math::Color	kCTColor			= { 0.4f, 0.7f, 1.0f, 1.0f };		// CTの残り

	constexpr float		kOpenSpeed			= 0.25f;	// 開く・閉じる速さ（1フレームに進む割合）
}

void ATBCommandMenu::Init()
{
	m_items.assign(static_cast<size_t>(AbilitySlot::Num), ItemState{});

	m_titleText = std::make_shared<UIText>();
	m_titleText->SetText("ATB コマンド", UIText::FontSize::Small);
	m_titleText->SetColor(kCursorMarkColor);
	m_titleText->SetCenterPos({ kNameLeftX + m_titleText->GetSize().x * 0.5f - 20.0f, kTitleY });

	m_helpText = std::make_shared<UIText>();
	m_helpText->SetText("W/S：選択　左クリック：決定　右クリック：戻る", UIText::FontSize::Small);
	m_helpText->SetColor(kDisableColor);
	m_helpText->SetScale(0.75f);
	m_helpText->SetCenterPos({ static_cast<float>(kPanelCenterX), kHelpY });

	for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
	{
		float y = kFirstItemY - kItemSpacing * i;

		auto name = std::make_shared<UIText>();
		name->SetText(kItemNames[i], UIText::FontSize::Small);
		name->SetCenterPos({ kNameLeftX + name->GetSize().x * 0.5f, y });
		m_nameTexts.push_back(name);

		auto cost = std::make_shared<UIText>();
		cost->SetText("ATB x" + std::to_string(i + 1), UIText::FontSize::Small);
		cost->SetCenterPos({ kCostRightX - cost->GetSize().x * 0.5f, y });
		m_costTexts.push_back(cost);
	}
}

void ATBCommandMenu::SetItemState(int index, bool canUse, float ctRatio, int cost)
{
	if (index < 0 || index >= static_cast<int>(m_items.size())) return;

	// 使う本数が変わったら文字を作り直す
	if (m_items[index].cost != cost && index < static_cast<int>(m_costTexts.size()))
	{
		m_costTexts[index]->SetText("ATB x" + std::to_string(cost), UIText::FontSize::Small);
		m_costTexts[index]->SetCenterPos({ kCostRightX - m_costTexts[index]->GetSize().x * 0.5f, kFirstItemY - kItemSpacing * index });
	}

	m_items[index].canUse = canUse;
	m_items[index].ctRatio = std::clamp(ctRatio, 0.0f, 1.0f);
	m_items[index].cost = cost;
}

void ATBCommandMenu::Update()
{
	m_frame++;

	// なめらかに開く・閉じる
	float target = m_isOpen ? 1.0f : 0.0f;
	m_openRate += (target - m_openRate) * kOpenSpeed;
	if (std::abs(target - m_openRate) < 0.01f) m_openRate = target;

	// 選べる項目は白、選べない項目は灰色
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		const Math::Color& color = m_items[i].canUse ? kEnableColor : kDisableColor;
		Math::Color c = color;
		c.w *= m_openRate;
		m_nameTexts[i]->SetColor(c);
		m_costTexts[i]->SetColor(c);
	}

	Math::Color titleColor = kCursorMarkColor;
	titleColor.w *= m_openRate;
	m_titleText->SetColor(titleColor);

	Math::Color helpColor = kDisableColor;
	helpColor.w *= m_openRate;
	m_helpText->SetColor(helpColor);
}

void ATBCommandMenu::DrawSprite()
{
	if (m_openRate <= 0.0f) return;

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// 開き具合に合わせて薄くする
	auto fade = [this](Math::Color color) { color.w *= m_openRate; return color; };

	// 画面全体を少し暗くして、時間がゆっくりになっていることを分かりやすくする
	Math::Color dim = fade(kDimColor);
	spriteShader.DrawBox(0, 0, 640, 360, &dim, true);

	// パネル（開く時は横に広がる）
	int halfW = static_cast<int>(kPanelHalfW * (0.6f + 0.4f * m_openRate));
	Math::Color panel = fade(kPanelColor);
	Math::Color frame = fade(kPanelFrameColor);
	spriteShader.DrawBox(kPanelCenterX, kPanelCenterY, halfW, kPanelHalfH, &panel, true);
	spriteShader.DrawBox(kPanelCenterX, kPanelCenterY, halfW, kPanelHalfH, &frame, false);

	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		int y = static_cast<int>(kFirstItemY - kItemSpacing * i);

		// 選んでいる項目：背景を光らせ、左に三角を出す（三角は少し左右に揺らす）
		if (i == m_cursor)
		{
			Math::Color cursor = fade(kCursorColor);
			spriteShader.DrawBox(kPanelCenterX, y, halfW - 6, kItemHalfH, &cursor, true);

			int markX = static_cast<int>(kNameLeftX) - 16 + static_cast<int>(std::sin(m_frame * 0.2f) * 3.0f);
			Math::Color mark = fade(kCursorMarkColor);
			spriteShader.DrawTriangle(markX - 6, y + 7, markX - 6, y - 7, markX + 6, y, &mark, true);
		}

		// CTの残り（項目の下に青い線。だんだん短くなる）
		if (m_items[i].ctRatio > 0.0f)
		{
			int width = static_cast<int>((kCostRightX - kNameLeftX) * m_items[i].ctRatio);
			if (width >= 2)
			{
				Math::Color ct = fade(kCTColor);
				spriteShader.DrawBox(static_cast<int>(kNameLeftX) + width / 2, y - kItemHalfH + 2, width / 2, kCTBarHeight / 2 + 1, &ct, true);
			}
		}
	}

	// 文字
	m_titleText->DrawSprite();
	for (const auto& text : m_nameTexts) text->DrawSprite();
	for (const auto& text : m_costTexts) text->DrawSprite();
	m_helpText->DrawSprite();
}
