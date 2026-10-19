#include "UIText.h"

namespace
{
	// フォント（KdFontManagerの登録番号。0番はデバッグ表示用に空けておく）
	// 本文（小・中）：太くて読みやすいゴシック体「Zen Kaku Gothic New」（日本語も英語も書ける）
	// ※SIL Open Font License（無料で使える。ライセンス文は Asset/Font/OFL_ZenKakuGothicNew.txt）
	const std::string	kFontName			= "Zen Kaku Gothic New";
	const std::string	kFontPath			= "Asset/Font/ZenKakuGothicNew-Bold.ttf";

	// 見出し（大）と、英語だけの文字（LabelSmall / LabelMedium）：Y224（英数字だけのフォント）
	const std::string	kLabelFontName		= "Y224";
	constexpr int		kFontNoLabelSmall	= 5;
	constexpr int		kFontNoLabelMedium	= 6;

	// 文字のふち（背景が明るくても暗くても読めるように、文字の周りに暗いふちを付ける）
	constexpr float		kOutlineWidth		= 2.0f;		// ふちの太さ（ピクセル）
	const Math::Color	kOutlineColor		= { 0.0f, 0.0f, 0.0f, 0.85f };
	constexpr int		kFontNoSmall		= 1;
	constexpr int		kFontNoMedium		= 2;
	constexpr int		kFontNoLarge		= 3;
	constexpr int		kFontNoTitle		= 4;
	constexpr int		kFontHeightSmall	= 24;
	constexpr int		kFontHeightMedium	= 40;
	constexpr int		kFontHeightLarge	= 96;
	constexpr int		kFontHeightTitle	= 100;

	// Y224 のフォントファイル（見出し・英語だけの文字に使う）
	const std::string	kY224FontPath		= "Asset/Font/Y224.ttf";

	// タイトルロゴ用：Y224（見出しと同じフォント）
	// ※Tools/MakeFont.py で作ったオリジナルフォント（Asset/Font/BraverStone.ttf）に戻す時は、ここを変える
	const std::string	kTitleFontName		= "Y224";

	constexpr int		kBlinkCycle = 60;	// 点滅の周期（フレーム）
}

void UIText::RegisterFonts()
{
	static bool isRegistered = false;
	if (isRegistered) return;

	// フォントファイルを読み込んでから登録する
	// （ファイルが無い場合はWindowsが似たフォントで代用する）
	for (const std::string* pPath : { &kFontPath, &kY224FontPath })
	{
		if (std::filesystem::exists(*pPath))
		{
			KdFontManager::Instance().AddFontResource(*pPath);
		}
		else
		{
			OutputDebugStringA(("【フォントファイルが見つかりません】: " + *pPath + "\n").c_str());
		}
	}

	// 文字ごとに幅が違うフォントなので、文字幅は指定しない（指定すると、条件に合う別のフォントが選ばれてしまう）
	KdFontManager::Instance().AddFont(kFontNoSmall, kFontName, kFontHeightSmall, SHIFTJIS_CHARSET, DEFAULT_PITCH | FF_DONTCARE);
	KdFontManager::Instance().AddFont(kFontNoMedium, kFontName, kFontHeightMedium, SHIFTJIS_CHARSET, DEFAULT_PITCH | FF_DONTCARE);
	// 見出しと英語だけの文字は Y224
	// Y224 は Shift-JIS に対応しておらず、文字ごとに幅が違うフォントなので、
	// 文字セット・文字幅を指定せずに「名前」を優先して探させる
	// （指定したままだと、条件に合う別のフォントが選ばれてしまう）
	KdFontManager::Instance().AddFont(kFontNoLarge, kLabelFontName, kFontHeightLarge, DEFAULT_CHARSET, DEFAULT_PITCH | FF_DONTCARE);
	KdFontManager::Instance().AddFont(kFontNoLabelSmall, kLabelFontName, kFontHeightSmall, DEFAULT_CHARSET, DEFAULT_PITCH | FF_DONTCARE);
	KdFontManager::Instance().AddFont(kFontNoLabelMedium, kLabelFontName, kFontHeightMedium, DEFAULT_CHARSET, DEFAULT_PITCH | FF_DONTCARE);

	// タイトルロゴも Y224
	KdFontManager::Instance().AddFont(kFontNoTitle, kTitleFontName, kFontHeightTitle, DEFAULT_CHARSET, DEFAULT_PITCH | FF_DONTCARE);
	isRegistered = true;
}

int UIText::GetFontNo(FontSize size)
{
	switch (size)
	{
	case FontSize::Small:	return kFontNoSmall;
	case FontSize::Large:	return kFontNoLarge;
	case FontSize::Title:	return kFontNoTitle;
	case FontSize::LabelSmall:	return kFontNoLabelSmall;
	case FontSize::LabelMedium:	return kFontNoLabelMedium;
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

	// ふち：同じ文字を暗い色で、上下左右・斜めに少しずらして先に描く
	if (m_isOutline)
	{
		Math::Color outlineColor = kOutlineColor;
		outlineColor.w *= m_color.w;
		for (int y = -1; y <= 1; ++y)
		{
			for (int x = -1; x <= 1; ++x)
			{
				if (x == 0 && y == 0) continue;
				Math::Vector2 offset = { x * kOutlineWidth, y * kOutlineWidth };
				spriteShader.DrawUIFont(m_fontSprite, pos + offset, &outlineColor, 0);
			}
		}
	}

	spriteShader.DrawUIFont(m_fontSprite, pos, &m_color, 0);

	// 他の描画に影響しないよう元に戻す
	spriteShader.SetMatrix(Math::Matrix::Identity);
}
