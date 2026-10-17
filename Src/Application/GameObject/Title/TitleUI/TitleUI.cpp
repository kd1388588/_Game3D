#include "TitleUI.h"
#include "../../../Utility/SpriteUtil.h"

#include <cmath>	// std::lerp

namespace
{
	// ===== 文字 =====
	const std::string	kLogoText			= "BRAVER";
	const std::string	kSubTitleText		= "- DUAL BLADE ACTION -";
	const std::vector<std::string> kMenuNames	= { "START", "CONTROLS", "EXIT" };	// MenuItem の順番

	// ===== 配置（画面中央が原点、上が+Y。キャラクターが右寄りに映るので左側にまとめる） =====
	const Math::Vector2	kLogoPos			= { -290.0f, 150.0f };
	constexpr float		kLogoScale			= 0.85f;
	constexpr float		kLineY				= 72.0f;		// ロゴの下の光の線
	constexpr float		kLineWidth			= 520.0f;
	const Math::Vector2	kSubTitlePos		= { -290.0f, 40.0f };
	constexpr float		kMenuX				= -290.0f;
	constexpr float		kMenuTopY			= -70.0f;
	constexpr float		kMenuSpacing		= 62.0f;
	constexpr float		kLetterBoxHeight	= 50.0f;		// 画面の上下の黒帯の太さ
	constexpr float		kGuideY				= -335.0f;		// 下の黒帯の中に操作案内を出す

	// ===== 登場演出のタイミング（フレーム） =====
	constexpr int		kFadeInFrame		= 45;			// 黒からのフェードイン
	constexpr int		kLetterBoxFrame		= 60;			// 黒帯が入ってくる
	constexpr int		kLogoStartFrame		= 30;			// ロゴが現れ始める
	constexpr int		kLogoAppearFrame	= 60;
	constexpr int		kLogoBurstFrame		= 35;			// ロゴが現れきった時の光
	constexpr int		kLineStartFrame		= 80;			// 光の線が伸び始める
	constexpr int		kLineAppearFrame	= 50;
	constexpr int		kSubTitleStartFrame	= 110;
	constexpr int		kSubTitleAppearFrame= 40;
	constexpr int		kMenuStartFrame		= 140;
	constexpr int		kMenuAppearFrame	= 45;
	constexpr int		kIntroEndFrame		= kMenuStartFrame + kMenuAppearFrame;

	// ===== 登場後も動き続ける演出 =====
	constexpr float		kLogoPulseSpeed		= 0.04f;		// ロゴの光の脈打つ速さ
	constexpr int		kSparkCycle			= 200;			// 光の線の上を走る光の周期
	constexpr int		kSparkMoveFrame		= 120;			// 光が線の端から端まで走るフレーム数
	constexpr float		kSmoothRate			= 0.2f;			// カーソルなどが目標に近づく割合（1フレーム）
	constexpr float		kSelectedScale		= 1.12f;		// 選択中の項目の拡大率
	constexpr float		kUnselectedScale	= 0.9f;
	constexpr float		kUnselectedAlpha	= 0.45f;

	// ===== 決定時 =====
	constexpr int		kDecideFlashFrame	= 30;			// 決定した項目を点滅させるフレーム数
	constexpr int		kFadeOutFrame		= 40;			// 黒へのフェードアウト

	// ===== 操作説明 =====
	constexpr float		kControlsHalfWidth	= 380.0f;
	constexpr float		kControlsHalfHeight	= 235.0f;
	constexpr float		kControlsKeyX		= -330.0f;		// キーの列の左端
	constexpr float		kControlsActionX	= -110.0f;		// 動作の列の左端
	constexpr float		kControlsTopY		= 120.0f;
	constexpr float		kControlsRowSpacing	= 34.0f;

	// ===== 色 =====
	const Math::Color	kLogoColor			= { 1.0f, 0.96f, 0.88f, 1.0f };
	const Math::Color	kLogoGlowColor		= { 1.0f, 0.6f, 0.25f, 1.0f };	// 火の粉と同じオレンジ
	const Math::Color	kAccentColor		= { 0.35f, 0.7f, 1.0f, 1.0f };	// 剣の軌跡と同じ青
	const Math::Color	kGuideColor			= { 0.75f, 0.75f, 0.75f, 1.0f };

	constexpr int		kGlowTexSize		= 64;
	constexpr int		kVignetteTexWidth	= 128;
	constexpr int		kVignetteTexHeight	= 72;
}

std::shared_ptr<UIText> TitleUI::CreateText(const std::string& text, UIText::FontSize fontSize)
{
	std::shared_ptr<UIText> uiText = std::make_shared<UIText>();
	uiText->SetText(text, fontSize);
	return uiText;
}

Math::Vector2 TitleUI::GetMenuItemPos(int index)
{
	return { kMenuX, kMenuTopY - kMenuSpacing * static_cast<float>(index) };
}

void TitleUI::AddControlsRow(const std::string& key, const std::string& action, float y)
{
	// UIText は中心の位置で表示するので、左端の位置に文字の幅の半分を足す
	std::shared_ptr<UIText> keyText = CreateText(key, UIText::FontSize::Small);
	keyText->SetCenterPos({ kControlsKeyX + keyText->GetSize().x * 0.5f, y });
	keyText->SetColor(kAccentColor);
	m_controlsTexts.push_back(keyText);

	std::shared_ptr<UIText> actionText = CreateText(action, UIText::FontSize::Small);
	actionText->SetCenterPos({ kControlsActionX + actionText->GetSize().x * 0.5f, y });
	m_controlsTexts.push_back(actionText);
}

void TitleUI::Init()
{
	m_screenSize = SpriteUtil::GetScreenSize();
	m_glowTex = SpriteUtil::CreateGlowTexture(kGlowTexSize);
	m_vignetteTex = SpriteUtil::CreateVignetteTexture(kVignetteTexWidth, kVignetteTexHeight);

	// ロゴ・サブタイトル
	m_logo = CreateText(kLogoText, UIText::FontSize::Title);
	m_logo->SetCenterPos(kLogoPos);

	m_subTitle = CreateText(kSubTitleText, UIText::FontSize::Small);
	m_subTitle->SetCenterPos(kSubTitlePos);

	// メニュー
	for (size_t i = 0; i < kMenuNames.size(); ++i)
	{
		std::shared_ptr<UIText> text = CreateText(kMenuNames[i], UIText::FontSize::Medium);
		text->SetCenterPos(GetMenuItemPos(static_cast<int>(i)));
		m_menuTexts.push_back(text);
	}
	m_menuScales.assign(m_menuTexts.size(), kUnselectedScale);
	m_cursorY = GetMenuItemPos(m_selectIndex).y;

	// 操作案内（下の黒帯）
	m_selectGuide = CreateText("W / S : SELECT        ENTER : DECIDE", UIText::FontSize::Small);
	m_selectGuide->SetCenterPos({ 0.0f, kGuideY });
	m_controlsGuide = CreateText("Enter / BackSpace：戻る", UIText::FontSize::Small);
	m_controlsGuide->SetCenterPos({ 0.0f, kGuideY });

	// 操作説明
	m_controlsTitle = CreateText("操作説明", UIText::FontSize::Medium);
	m_controlsTitle->SetCenterPos({ 0.0f, 180.0f });

	const std::vector<std::pair<std::string, std::string>> controls =
	{
		{ "W A S D",		"移動" },
		{ "マウス",			"カメラを回す" },
		{ "E",				"バトルモード切り替え" },
		{ "左クリック",		"攻撃（続けて押すとコンボ）" },
		{ "右クリック",		"回避" },
		{ "F",				"ATBコマンド（スローにして技を選ぶ）" },
		{ "Shift",			"走りながら：ダッシュ　止まって：ガード" },
		{ "Space",			"ジャンプ" },
		{ "G",				"覚醒（WPゲージが満タンの時）" },
	};
	for (size_t i = 0; i < controls.size(); ++i)
	{
		AddControlsRow(controls[i].first, controls[i].second, kControlsTopY - kControlsRowSpacing * static_cast<float>(i));
	}
}

// ===================================================================
// 更新
// ===================================================================

void TitleUI::UpdateInput()
{
	// || でまとめると後ろのキーの Update() が呼ばれないことがあるので、全て先に呼ぶ
	bool up			= m_upKey.Update();
	bool upArrow	= m_upArrowKey.Update();
	bool down		= m_downKey.Update();
	bool downArrow	= m_downArrowKey.Update();
	bool enter		= m_enterKey.Update();
	bool space		= m_spaceKey.Update();
	bool click		= m_clickKey.Update();
	bool backSpace	= m_backKey.Update();
	bool rightClick	= m_rightClickKey.Update();

	m_input.up		= up || upArrow;
	m_input.down	= down || downArrow;
	m_input.decide	= enter || space || click;
	m_input.back	= backSpace || rightClick;
}

void TitleUI::ChangeState(State state)
{
	m_state = state;
	m_stateFrame = 0;
}

void TitleUI::Update()
{
	m_frame++;
	m_stateFrame++;

	UpdateInput();

	switch (m_state)
	{
	case State::Intro:		UpdateIntro();		break;
	case State::Select:		UpdateSelect();		break;
	case State::Controls:	UpdateControls();	break;
	case State::Decided:	UpdateDecided();	break;
	}

	UpdateMenuAnimation();
}

void TitleUI::UpdateIntro()
{
	// 決定ボタンで登場演出を飛ばす
	if (m_input.decide)
	{
		m_frame = std::max(m_frame, kIntroEndFrame);
	}

	if (m_frame >= kIntroEndFrame)
	{
		ChangeState(State::Select);
	}
}

void TitleUI::UpdateSelect()
{
	int itemNum = static_cast<int>(MenuItem::Num);

	if (m_input.up)		{ m_selectIndex = (m_selectIndex + itemNum - 1) % itemNum; }
	if (m_input.down)	{ m_selectIndex = (m_selectIndex + 1) % itemNum; }

	if (m_input.decide)
	{
		Decide(static_cast<MenuItem>(m_selectIndex));
	}
}

void TitleUI::UpdateControls()
{
	if (m_input.decide || m_input.back)
	{
		ChangeState(State::Select);
	}
}

void TitleUI::UpdateDecided()
{
	// 点滅・フェードアウトが終わったら結果を伝える
	if (m_stateFrame < kDecideFlashFrame + kFadeOutFrame) return;

	m_result = (m_decidedItem == MenuItem::Exit) ? Result::Exit : Result::Start;
}

void TitleUI::Decide(MenuItem item)
{
	if (item == MenuItem::Controls)
	{
		ChangeState(State::Controls);
		return;
	}

	m_decidedItem = item;
	ChangeState(State::Decided);
}

void TitleUI::UpdateMenuAnimation()
{
	// カーソルを選択中の項目へなめらかに動かす
	float targetY = GetMenuItemPos(m_selectIndex).y;
	m_cursorY += (targetY - m_cursorY) * kSmoothRate;

	// 選択中の項目だけ少し大きくする
	for (size_t i = 0; i < m_menuScales.size(); ++i)
	{
		float target = (static_cast<int>(i) == m_selectIndex) ? kSelectedScale : kUnselectedScale;
		m_menuScales[i] += (target - m_menuScales[i]) * kSmoothRate;
	}

	// 操作説明の開き具合
	float openTarget = (m_state == State::Controls) ? 1.0f : 0.0f;
	m_controlsOpen += (openTarget - m_controlsOpen) * kSmoothRate;
}

float TitleUI::CalcProgress(int startFrame, int duration) const
{
	return std::clamp(static_cast<float>(m_frame - startFrame) / duration, 0.0f, 1.0f);
}

// ===================================================================
// 描画
// ===================================================================

void TitleUI::DrawSprite()
{
	DrawVignette();
	DrawLetterBox();
	DrawLogo();
	DrawMenu();
	DrawControls();
	DrawFade();
}

void TitleUI::DrawGlow(const Math::Vector2& center, const Math::Vector2& size, const Math::Color& color) const
{
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_glowTex.get(),
		static_cast<int>(center.x), static_cast<int>(center.y),
		static_cast<int>(size.x), static_cast<int>(size.y), nullptr, &color);
}

void TitleUI::DrawVignette() const
{
	if (!m_vignetteTex) return;

	// 画面の四隅を暗くして、中央に目が行くようにする
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_vignetteTex.get(), 0, 0,
		static_cast<int>(m_screenSize.x), static_cast<int>(m_screenSize.y));
}

void TitleUI::DrawLetterBox() const
{
	// 画面の外から黒帯が入ってくる
	float progress = SpriteUtil::EaseOutCubic(CalcProgress(0, kLetterBoxFrame));
	float height = kLetterBoxHeight * progress;
	if (height <= 0.0f) return;

	int halfW = static_cast<int>(m_screenSize.x * 0.5f);
	int halfH = static_cast<int>(height * 0.5f);
	int topY = static_cast<int>(m_screenSize.y * 0.5f - height * 0.5f);
	Math::Color black = { 0.0f, 0.0f, 0.0f, 1.0f };

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.DrawBox(0, topY, halfW, halfH, &black);
	spriteShader.DrawBox(0, -topY, halfW, halfH, &black);

	// 下の黒帯の操作案内（ゆっくり明滅させる）
	if (m_state == State::Intro || m_state == State::Decided) return;

	std::shared_ptr<UIText> guide = (m_state == State::Controls) ? m_controlsGuide : m_selectGuide;
	Math::Color color = kGuideColor;
	color.w = 0.55f + 0.35f * std::sin(m_frame * 0.06f);
	guide->SetColor(color);
	guide->DrawSprite();
}

void TitleUI::DrawLogo() const
{
	auto& shaderManager = KdShaderManager::Instance();

	float appear = CalcProgress(kLogoStartFrame, kLogoAppearFrame);
	if (appear <= 0.0f) return;

	float ease = SpriteUtil::EaseOutCubic(appear);
	float pulse = std::sin(m_frame * kLogoPulseSpeed);

	// ---- 加算合成で光らせるもの ----
	shaderManager.ChangeBlendState(KdBlendState::Add);
	{
		// ロゴの後ろの大きな光
		Math::Color backGlow = kLogoGlowColor;
		backGlow.w = (0.22f + 0.06f * pulse) * ease;
		DrawGlow(kLogoPos, { 760.0f, 300.0f }, backGlow);

		// ロゴの下の光の線（伸びていく）
		float lineProgress = SpriteUtil::EaseOutCubic(CalcProgress(kLineStartFrame, kLineAppearFrame));
		float lineWidth = kLineWidth * lineProgress;
		if (lineWidth > 1.0f)
		{
			Math::Color lineColor = kAccentColor;
			lineColor.w = 0.9f;
			DrawGlow({ kLogoPos.x, kLineY }, { lineWidth, 10.0f }, lineColor);
			DrawGlow({ kLogoPos.x, kLineY }, { lineWidth * 0.6f, 4.0f }, { 1.0f, 1.0f, 1.0f, 0.8f });

			// 線の上を左から右へ走る光
			int sparkFrame = (m_frame - kLineStartFrame) % kSparkCycle;
			if (lineProgress >= 1.0f && sparkFrame < kSparkMoveFrame)
			{
				float s = static_cast<float>(sparkFrame) / kSparkMoveFrame;
				float x = kLogoPos.x - lineWidth * 0.5f + lineWidth * s;
				float bright = std::sin(s * DirectX::XM_PI);
				DrawGlow({ x, kLineY }, { 90.0f, 26.0f }, { 1.0f, 1.0f, 1.0f, bright });
			}
		}

		// ロゴに重ねて脈打つ光
		Math::Color glow = kLogoGlowColor;
		glow.w = (0.3f + 0.15f * pulse) * ease;
		m_logo->SetColor(glow);
		m_logo->SetScale(kLogoScale * (1.02f + 0.01f * pulse));
		m_logo->DrawSprite();

		// ロゴが現れきった瞬間に、ロゴが広がりながら消える光
		float burst = CalcProgress(kLogoStartFrame + kLogoAppearFrame, kLogoBurstFrame);
		if (burst > 0.0f && burst < 1.0f)
		{
			m_logo->SetColor({ 1.0f, 0.9f, 0.7f, (1.0f - burst) * 0.9f });
			m_logo->SetScale(kLogoScale * (1.0f + 0.3f * SpriteUtil::EaseOutCubic(burst)));
			m_logo->DrawSprite();
		}
	}
	shaderManager.UndoBlendState();

	// ---- ロゴ本体（大きい状態から縮みながら現れる） ----
	Math::Color logoColor = kLogoColor;
	logoColor.w = ease;
	m_logo->SetColor(logoColor);
	m_logo->SetScale(kLogoScale * std::lerp(1.35f, 1.0f, ease));
	m_logo->DrawSprite();

	// ---- サブタイトル ----
	float subAlpha = CalcProgress(kSubTitleStartFrame, kSubTitleAppearFrame);
	if (subAlpha > 0.0f)
	{
		m_subTitle->SetColor({ 0.85f, 0.85f, 0.85f, subAlpha });
		m_subTitle->DrawSprite();
	}
}

void TitleUI::DrawMenu() const
{
	float appear = SpriteUtil::EaseOutCubic(CalcProgress(kMenuStartFrame, kMenuAppearFrame));
	if (appear <= 0.0f) return;

	// 操作説明を開いている間はメニューを暗くする
	float dim = 1.0f - 0.7f * m_controlsOpen;

	// 決定後は、決定した項目を点滅させてから全体を消す
	bool isDecided = (m_state == State::Decided);
	bool flashOn = isDecided && (m_stateFrame < kDecideFlashFrame) && ((m_stateFrame / 3) % 2 == 0);

	// 左から滑り込んでくる
	float slideX = -40.0f * (1.0f - appear);

	// ---- 選択中の項目の後ろの光とカーソル（加算合成） ----
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeBlendState(KdBlendState::Add);
	{
		float pulse = std::sin(m_frame * 0.08f);
		Math::Vector2 cursorPos = { kMenuX + slideX, m_cursorY };

		Math::Color barColor = kAccentColor;
		barColor.w = ((flashOn ? 0.6f : 0.22f) + 0.06f * pulse) * appear * dim;
		DrawGlow(cursorPos, { 440.0f, 70.0f }, barColor);

		// 項目の左右で揺れる光の点
		float halfWidth = m_menuTexts[m_selectIndex]->GetSize().x * 0.5f * m_menuScales[m_selectIndex];
		float offset = halfWidth + 34.0f + 6.0f * pulse;
		Math::Color dotColor = { 1.0f, 1.0f, 1.0f, appear * dim };
		DrawGlow({ cursorPos.x - offset, cursorPos.y }, { 26.0f, 26.0f }, dotColor);
		DrawGlow({ cursorPos.x + offset, cursorPos.y }, { 26.0f, 26.0f }, dotColor);
	}
	shaderManager.UndoBlendState();

	// ---- 項目の文字 ----
	for (size_t i = 0; i < m_menuTexts.size(); ++i)
	{
		bool isSelected = (static_cast<int>(i) == m_selectIndex);
		float alpha = isSelected ? 1.0f : kUnselectedAlpha;
		if (isSelected && isDecided && m_stateFrame < kDecideFlashFrame && !flashOn) { alpha = 0.4f; }

		Math::Vector2 pos = GetMenuItemPos(static_cast<int>(i));
		m_menuTexts[i]->SetCenterPos({ pos.x + slideX, pos.y });
		m_menuTexts[i]->SetScale(m_menuScales[i]);
		m_menuTexts[i]->SetColor({ 1.0f, 1.0f, 1.0f, alpha * appear * dim });
		m_menuTexts[i]->DrawSprite();
	}
}

void TitleUI::DrawControls() const
{
	if (m_controlsOpen <= 0.01f) return;

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	// 上下に開く黒い板
	int halfW = static_cast<int>(kControlsHalfWidth);
	int halfH = static_cast<int>(kControlsHalfHeight * m_controlsOpen);
	Math::Color boardColor = { 0.0f, 0.02f, 0.05f, 0.85f * m_controlsOpen };
	spriteShader.DrawBox(0, 0, halfW, halfH, &boardColor);

	// 上下のふちの青い線
	Math::Color lineColor = kAccentColor;
	lineColor.w = m_controlsOpen;
	spriteShader.DrawLine(-halfW, halfH, halfW, halfH, &lineColor);
	spriteShader.DrawLine(-halfW, -halfH, halfW, -halfH, &lineColor);

	// 文字は板が開ききる直前から表示する
	float textAlpha = SpriteUtil::SmoothStep(0.7f, 1.0f, m_controlsOpen);
	if (textAlpha <= 0.0f) return;

	m_controlsTitle->SetColor({ 1.0f, 1.0f, 1.0f, textAlpha });
	m_controlsTitle->DrawSprite();

	for (const auto& text : m_controlsTexts)
	{
		text->SetAlpha(textAlpha);
		text->DrawSprite();
	}
}

void TitleUI::DrawFade() const
{
	// 最初は黒からフェードイン、決定後は黒へフェードアウト
	float alpha = 1.0f - CalcProgress(0, kFadeInFrame);
	if (m_state == State::Decided)
	{
		float fadeOut = std::clamp(static_cast<float>(m_stateFrame - kDecideFlashFrame) / kFadeOutFrame, 0.0f, 1.0f);
		alpha = std::max(alpha, fadeOut);
	}
	if (alpha <= 0.0f) return;

	Math::Color black = { 0.0f, 0.0f, 0.0f, alpha };
	KdShaderManager::Instance().m_spriteShader.DrawBox(0, 0,
		static_cast<int>(m_screenSize.x * 0.5f) + 1, static_cast<int>(m_screenSize.y * 0.5f) + 1, &black);
}
