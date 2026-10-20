#include "UIText.h"

namespace
{
	// フォント（KdFontManagerの登録番号。0番はデバッグ表示用に空けておく）
	// 本文（小・中）：日本語も英語も書ける明朝体「Zen Old Mincho」（遺跡・ダークファンタジーの雰囲気に合わせる）
	// 見出し（大）　：ローマの碑文のような英字のフォント「Cinzel」（GAME CLEAR / GAME OVER など英字だけ）
	// ※どちらも SIL Open Font License（無料で使える。ライセンス文は Asset/Font/OFL_*.txt）
	const std::string	kFontName			= "Zen Old Mincho";
	const std::string	kFontPath			= "Asset/Font/ZenOldMincho-Bold.ttf";
	// ※元の Cinzel.ttf は可変フォントで、Windows の文字の描画では「M」などの輪郭が重なる所が欠けて表示される
	//   そこで Tools/MakeHeadingFont.py で、太さを決めて輪郭の重なりをまとめた CinzelStatic.ttf を作って使う
	const std::string	kHeadingFontName	= "Cinzel Static";
	const std::string	kHeadingFontPath	= "Asset/Font/CinzelStatic.ttf";
	constexpr int		kFontNoSmall		= 1;
	constexpr int		kFontNoMedium		= 2;
	constexpr int		kFontNoLarge		= 3;
	constexpr int		kFontNoTitle		= 4;
	constexpr int		kFontHeightSmall	= 24;
	constexpr int		kFontHeightMedium	= 40;
	constexpr int		kFontHeightLarge	= 96;
	constexpr int		kFontHeightTitle	= 100;

	// タイトルロゴ用のフォント（Asset/Font に置いたフォントファイルを読み込んで使う）
	const std::string	kTitleFontPath		= "Asset/Font/Y224.ttf";
	const std::string	kTitleFontName		= "Y224";	// ファイル名ではなく、フォントの中に書かれている名前

	constexpr int		kBlinkCycle = 60;	// 点滅の周期（フレーム）
}

void UIText::RegisterFonts()
{
	static bool isRegistered = false;
	if (isRegistered) return;

	// フォントファイルを読み込んでから登録する
	// （ファイルが無い場合はWindowsが似たフォントで代用する）
	for (const std::string* pPath : { &kFontPath, &kHeadingFontPath, &kTitleFontPath })
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
	// Cinzel は日本語の文字が無いので、文字セットも指定しない
	KdFontManager::Instance().AddFont(kFontNoLarge, kHeadingFontName, kFontHeightLarge, DEFAULT_CHARSET, DEFAULT_PITCH | FF_DONTCARE);

	// Y224 は Shift-JIS に対応しておらず、文字ごとに幅が違うフォントなので、
	// 文字セット・文字幅を指定せずに「名前」を優先して探させる
	// （指定したままだと、条件に合う別のフォントが選ばれてしまう）
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
	spriteShader.DrawUIFont(m_fontSprite, pos, &m_color, 0);

	// 他の描画に影響しないよう元に戻す
	spriteShader.SetMatrix(Math::Matrix::Identity);
}
