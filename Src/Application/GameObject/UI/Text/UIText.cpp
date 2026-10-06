#include "UIText.h"

namespace
{
	// フォント（KdFontManagerの登録番号。0番はデバッグ表示用に空けておく）
	const std::string	kFontName			= "Arial";
	constexpr int		kFontNoSmall		= 1;
	constexpr int		kFontNoMedium		= 2;
	constexpr int		kFontNoLarge		= 3;
	constexpr int		kFontHeightSmall	= 24;
	constexpr int		kFontHeightMedium	= 40;
	constexpr int		kFontHeightLarge	= 96;

	constexpr int		kBlinkCycle			= 60;	// 点滅の周期（フレーム）
}

void UIText::RegisterFonts()
{
	static bool isRegistered = false;
	if (isRegistered) return;

	KdFontManager::Instance().AddFont(kFontNoSmall, kFontName, kFontHeightSmall);
	KdFontManager::Instance().AddFont(kFontNoMedium, kFontName, kFontHeightMedium);
	KdFontManager::Instance().AddFont(kFontNoLarge, kFontName, kFontHeightLarge);
	isRegistered = true;
}

int UIText::GetFontNo(FontSize size)
{
	switch (size)
	{
	case FontSize::Small:	return kFontNoSmall;
	case FontSize::Large:	return kFontNoLarge;
	default:				return kFontNoMedium;
	}
}

void UIText::SetText(const std::string& text, FontSize size)
{
	RegisterFonts();
	m_fontSprite = KdFontManager::Instance().CreateFontTexture(GetFontNo(size), text, 0);
}

void UIText::Update()
{
	if (m_isBlink)
	{
		m_blinkTimer = (m_blinkTimer + 1) % kBlinkCycle;
	}
}

void UIText::DrawSprite()
{
	if (!m_fontSprite || m_fontSprite->GetTexList().empty()) return;

	// 点滅中は周期の後半を非表示にする
	if (m_isBlink && m_blinkTimer >= kBlinkCycle / 2) return;

	// DrawFontは左下が基準なので、中心が m_centerPos になるようずらす
	float width = static_cast<float>(m_fontSprite->GetTotalWidth());
	float height = static_cast<float>(m_fontSprite->GetTexList()[0]->FontTex->GetInfo().Height);
	Math::Vector2 pos = { m_centerPos.x - width * 0.5f, m_centerPos.y - height * 0.5f };

	KdShaderManager::Instance().m_spriteShader.DrawFont(m_fontSprite, pos, &m_color, 0);
}
