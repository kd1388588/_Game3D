#include "ATBCommandMenu.h"
#include "../../../Player/Player.h"
#include "../../../../UI/Text/UIText.h"
#include "../Base/HudDraw.h"

namespace
{
	// 選べる技（並びは AbilitySlot と同じ）
	const char* const	kItemNames[]		= { "Attack1", "Attack2" };	// 上から（PlayerConstants.h の kAbilityInfos と同じ順番）

	// 位置・大きさ（FF7R 風。画面の左下に、斜めの行を縦に並べる）
	constexpr int		kPanelCenterX		= -440;		// 後ろの板の中心
	constexpr int		kPanelCenterY		= -262;
	constexpr int		kPanelHalfW			= 180;
	constexpr int		kPanelHalfH			= 62;
	constexpr float		kTitleY				= -186.0f;
	constexpr float		kFirstItemY			= -232.0f;
	constexpr float		kItemSpacing		= 42.0f;
	constexpr float		kHelpY				= -338.0f;
	constexpr float		kNameLeftX			= -585.0f;	// 技の名前の左端
	constexpr float		kCostRightX			= -290.0f;	// 使うゲージの本数の右端
	constexpr int		kItemHalfH			= 17;		// 行の高さ（半分）
	constexpr float		kItemLeftX			= -605.0f;	// 行の左端
	constexpr float		kItemRightX			= -270.0f;	// 行の右端
	constexpr int		kCTBarHeight		= 2;		// CTの残りのバーの太さ
	constexpr float		kTitleScale			= 0.6f;
	constexpr float		kCostSegW			= 16.0f;	// 使うゲージの本数の、斜めの印1つの幅
	constexpr float		kCostSegH			= 6.0f;

	// 色
	const Math::Color	kDimColor			= { 0.02f, 0.1f, 0.25f, 0.3f };	// 画面全体を青っぽく暗くする（ウェイトモード）
	const Math::Color	kPanelLeftColor		= { 0.02f, 0.06f, 0.14f, 0.85f };	// 後ろの板（左）
	const Math::Color	kPanelRightColor	= { 0.04f, 0.12f, 0.26f, 0.0f };	// 後ろの板（右は透明に溶ける）
	const Math::Color	kPanelLineColor		= { 0.55f, 0.82f, 1.0f, 0.55f };	// 板の上の明るい線
	const Math::Color	kCursorColor		= { 1.0f, 1.0f, 1.0f, 0.95f };		// 選んでいる行（左：白）
	const Math::Color	kCursorFadeColor	= { 0.6f, 0.88f, 1.0f, 0.0f };		// 選んでいる行（右：透明な水色）
	const Math::Color	kCursorMarkColor	= { 1.0f, 1.0f, 1.0f, 1.0f };		// 選んでいる行の左の三角
	const Math::Color	kTitleColor			= { 0.65f, 0.88f, 1.0f, 1.0f };		// 「ATB COMMAND」
	const Math::Color	kSelectTextColor	= { 0.02f, 0.08f, 0.16f, 1.0f };	// 選んでいる行の文字（白い帯の上なので暗い紺）
	const Math::Color	kEnableColor		= { 1.0f, 1.0f, 1.0f, 1.0f };		// 選べる項目の文字
	const Math::Color	kDisableColor		= { 0.45f, 0.48f, 0.55f, 1.0f };	// 選べない項目の文字
	const Math::Color	kCTColor			= { 0.4f, 0.75f, 1.0f, 1.0f };		// CTの残り
	const Math::Color	kCostColor			= { 0.45f, 0.85f, 1.0f, 1.0f };		// 使うゲージの本数の印
	const Math::Color	kCostOffColor		= { 0.45f, 0.85f, 1.0f, 0.25f };

	constexpr float		kOpenSpeed			= 0.25f;	// 開く・閉じる速さ（1フレームに進む割合）
}

void ATBCommandMenu::Init()
{
	m_items.assign(static_cast<size_t>(AbilitySlot::Num), ItemState{});

	m_titleText = std::make_shared<UIText>();
	m_titleText->SetText("ATB COMMAND", UIText::FontSize::Small);
	m_titleText->SetColor(kTitleColor);
	m_titleText->SetScale(kTitleScale);
	m_titleText->SetCenterPos({ kItemLeftX + 10.0f + m_titleText->GetSize().x * kTitleScale * 0.5f, kTitleY });

	m_helpText = std::make_shared<UIText>();
	m_helpText->SetText("W/S：選択　左クリック：決定　右クリック：戻る", UIText::FontSize::Small);
	m_helpText->SetColor(kDisableColor);
	m_helpText->SetScale(0.75f);
	m_helpText->SetCenterPos({ kItemLeftX + 10.0f + m_helpText->GetSize().x * 0.75f * 0.5f, kHelpY });

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

	// 選べる項目は白、選べない項目は灰色。選んでいる行は白い帯の上なので暗い紺
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		const Math::Color& color = (i == m_cursor) ? kSelectTextColor : (m_items[i].canUse ? kEnableColor : kDisableColor);
		Math::Color c = color;
		if (i == m_cursor && !m_items[i].canUse) c.w *= 0.55f;	// 選んでいても使えない時は薄く
		c.w *= m_openRate;
		m_nameTexts[i]->SetColor(c);
		m_costTexts[i]->SetColor(c);
	}

	Math::Color titleColor = kTitleColor;
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

	// 画面全体を青っぽく暗くして、時間がゆっくりになっていること（ウェイトモード）を分かりやすくする
	Math::Color dim = fade(kDimColor);
	spriteShader.DrawBox(0, 0, 640, 360, &dim, true);

	// 後ろの斜めの板（左は濃く、右へ透明に溶ける。開く時は左から伸びる）
	float open = 0.6f + 0.4f * m_openRate;
	float panelHalfW = kPanelHalfW * open;
	float panelCX = kPanelCenterX - kPanelHalfW + panelHalfW;
	const float pcy = static_cast<float>(kPanelCenterY);
	HudDraw::SlantGradient(panelCX, pcy, panelHalfW, kPanelHalfH, fade(kPanelLeftColor), fade(kPanelRightColor), HudDraw::kPanelSlant, pcy);
	HudDraw::SlantRect(panelCX, pcy + kPanelHalfH - 0.5f, panelHalfW, 1.0f, fade(kPanelLineColor), HudDraw::kPanelSlant, pcy);

	const float itemHalfW = (kItemRightX - kItemLeftX) * 0.5f;
	const float itemCX = kItemLeftX + itemHalfW;
	for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
	{
		float y = kFirstItemY - kItemSpacing * i;

		// 選んでいる行：白から透明へのグラデーションの斜めの帯と、左の白い三角（少し左右に揺らす）
		if (i == m_cursor)
		{
			HudDraw::SlantGradient(itemCX, y, itemHalfW, static_cast<float>(kItemHalfH),
				fade(kCursorColor), fade(kCursorFadeColor), HudDraw::kPanelSlant, y, 24);

			int markX = static_cast<int>(kItemLeftX) - 20 + static_cast<int>(std::sin(m_frame * 0.2f) * 3.0f);
			int iy = static_cast<int>(y);
			Math::Color mark = fade(kCursorMarkColor);
			spriteShader.DrawTriangle(markX - 8, iy + 10, markX - 8, iy - 10, markX + 8, iy, &mark, true);
		}

		// 使うゲージの本数（右側に、斜めの印を本数だけ並べる）
		for (int k = 0; k < m_items[i].cost; ++k)
		{
			float segX = kCostRightX - (m_items[i].cost - 1 - k) * (kCostSegW + 4.0f) - kCostSegW * 0.5f;
			const Math::Color& seg = m_items[i].canUse ? kCostColor : kCostOffColor;
			HudDraw::SlantRect(segX, y, kCostSegW * 0.5f, kCostSegH * 0.5f, fade(seg), HudDraw::kGaugeSlant, y);
		}

		// CTの残り（行の下に細い線。だんだん短くなる）
		if (m_items[i].ctRatio > 0.0f)
		{
			float width = (kItemRightX - kItemLeftX - 30.0f) * m_items[i].ctRatio;
			if (width >= 2.0f)
			{
				float barY = y - kItemHalfH + 3.0f;
				HudDraw::SlantRect(kItemLeftX + 15.0f + width * 0.5f, barY, width * 0.5f, kCTBarHeight * 0.5f, fade(kCTColor), HudDraw::kPanelSlant, y);
			}
		}
	}
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// 文字（使うゲージの本数は、文字ではなく上の印で表す）
	m_titleText->DrawSprite();
	for (const auto& text : m_nameTexts) text->DrawSprite();
	m_helpText->DrawSprite();
}
