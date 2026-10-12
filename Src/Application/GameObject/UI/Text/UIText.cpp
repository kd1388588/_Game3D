#include "UIText.h"

namespace
{
	// フォント（KdFontManagerの登録番号。0番はデバッグ表示用に空けておく）
	const std::string	kFontName			= "Arial";
	constexpr int		kFontNoSmall		= 1;
	constexpr int		kFontNoMedium		= 2;
	constexpr int		kFontNoLarge		= 3;
	constexpr int		kFontNoTitle		= 4;
	constexpr int		kFontHeightSmall	= 24;
	constexpr int		kFontHeightMedium	= 40;
	constexpr int		kFontHeightLarge	= 96;
	constexpr int		kFontHeightTitle	= 150;

	// タイトルロゴ用のフォント（見つからない場合はWindowsが似たフォントで代用する）
	const std::string	kTitleFontName		= "Times New Roman";

	constexpr int		kBlinkCycle			= 60;	// 点滅の周期（フレーム）
}

void UIText::RegisterFonts()
{
	static bool isRegistered = false;
	if (isRegistered) return;

	KdFontManager::Instance().AddFont(kFontNoSmall, kFontName, kFontHeightSmall);
	KdFontManager::Instance().AddFont(kFontNoMedium, kFontName, kFontHeightMedium);
	KdFontManager::Instance().AddFont(kFontNoLarge, kFontName, kFontHeightLarge);
	KdFontManager::Instance().AddFont(kFontNoTitle, kTitleFontName, kFontHeightTitle);
	isRegistered = true;
}

int UIText::GetFontNo(FontSize size)
{
	switch (size)
	{
	case FontSize::Small:	return kFontNoSmall;
	case FontSize::Large:	return kFontNoLarge;
	case FontSize::Title:	return kFontNoTitle;
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

Math::Vector2 UIText::GetSize() const
{
	if (!m_fontSprite || m_fontSprite->GetTexList().empty()) return Math::Vector2::Zero;

	float width = static_cast<float>(m_fontSprite->GetTotalWidth());
	float height = static_cast<float>(m_fontSprite->GetTexList()[0]->FontTex->GetInfo().Height);
	return { width, height };
}

void UIText::DrawSprite()
{
	if (!m_fontSprite || m_fontSprite->GetTexList().empty()) return;

	// 点滅中は周期の後半を非表示にする
	if (m_isBlink && m_blinkTimer >= kBlinkCycle / 2) return;

	// DrawFontは左下が基準なので、中心が原点になるようずらしてから、
	// 拡大・移動の行列で m_centerPos に表示する
	Math::Vector2 size = GetSize();
	Math::Vector2 pos = { -size.x * 0.5f, -size.y * 0.5f };

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::CreateScale(m_scale) *
		Math::Matrix::CreateTranslation(m_centerPos.x, m_centerPos.y, 0.0f));
	spriteShader.DrawFont(m_fontSprite, pos, &m_color, 0);

	// 他の描画に影響しないよう元に戻す
	spriteShader.SetMatrix(Math::Matrix::Identity);
}
