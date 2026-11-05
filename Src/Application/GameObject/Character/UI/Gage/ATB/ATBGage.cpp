#include "ATBGage.h"
#include "../../../Player/Player.h"
#include "../../../../UI/Text/UIText.h"
#include "../Base/HudDraw.h"
#include "../HP/HPGage.h"

namespace
{
	// 位置・大きさ（FF7R 風。画面の右下のステータスパネル（HPGage が描く）の中に並べる）
	// パネル：中心 (371, -293)、幅 470・高さ 66。左から「モード」「HP（HPGage）」「ATB」
	constexpr int		kGageLeft			= 525;		// ATBゲージ1本目の左端
	constexpr int		kGageY				= -300;		// ATBゲージの中心の高さ
	constexpr int		kBarWidth			= 34;		// ATBゲージ1本の幅
	constexpr int		kBarHeight			= 8;		// ATBゲージ1本の高さ
	constexpr int		kBarSpace			= 6;		// ゲージとゲージの間
	constexpr float		kAtbLabelY			= -280.0f;	// 「ATB」の文字の高さ
	constexpr float		kAtbLabelScale		= 0.55f;
	constexpr float		kModeLeftX			= 176.0f;	// モードの文字（BLADE / SLASH）の左端
	constexpr float		kModeTextY			= -281.0f;	// モードの文字の高さ
	constexpr float		kModeTextScale		= 0.9f;
	constexpr int		kModeLockBarY		= -297;		// モードの固定の残り時間のバーの高さ
	constexpr int		kModeLockBarHeight	= 2;
	constexpr float		kAccentX			= 158.0f;	// パネルの左の、モードの色の斜めの線
	constexpr float		kSubTextY			= -308.0f;	// GUARD / BREAK の文字の高さ
	constexpr float		kSubTextScale		= 0.6f;

	// アビリティの枠（ステータスパネルの右上）
	constexpr int		kSlotCenterX		= 548;		// 1つ目の枠の中心
	constexpr int		kSlotSpace			= 42;		// 枠と枠の間隔
	constexpr int		kSlotCenterY		= -238;
	constexpr int		kSlotHalfSize		= 17;
	constexpr int		kSlotCostDotSize	= 2;		// 使うゲージの本数を表す点の大きさ（半分）


	// 色
	const Math::Color	kBackColor			= { 0.02f, 0.06f, 0.14f, 0.8f };	// 背景
	const Math::Color	kFrameColor			= { 0.55f, 0.78f, 1.0f, 0.45f };	// 枠
	const Math::Color	kFillColor			= { 0.16f, 0.4f, 0.85f, 1.0f };		// たまっている途中（暗めの青）
	const Math::Color	kFullColor			= { 0.45f, 0.85f, 1.0f, 1.0f };		// 満タン（明るい水色。光らせる）
	const Math::Color	kAtbLabelColor		= { 0.65f, 0.85f, 1.0f, 0.8f };
	const Math::Color	kOperatorColor		= { 0.45f, 0.8f, 1.0f, 1.0f };		// OPERATOR の文字
	const Math::Color	kPunisherColor		= { 1.0f, 0.6f, 0.2f, 1.0f };		// PUNISHER の文字
	const Math::Color	kLockColor			= { 0.9f, 0.3f, 0.3f, 1.0f };		// モードの固定の残り時間
	const Math::Color	kSlotReadyColor		= { 1.0f, 0.85f, 0.3f, 1.0f };		// 使えるアビリティの枠
	const Math::Color	kSlotDisableColor	= { 0.4f, 0.4f, 0.45f, 0.9f };		// 使えないアビリティの枠
	const Math::Color	kSlotCTColor		= { 0.0f, 0.0f, 0.0f, 0.65f };		// CTの残り（上から暗くする）
	const Math::Color	kGuardColor			= { 0.85f, 0.95f, 1.0f, 1.0f };		// GUARD の文字
	const Math::Color	kBreakColor			= { 1.0f, 0.3f, 0.25f, 1.0f };		// BREAK の文字
	const Math::Color	kGuardBarColor		= { 0.6f, 0.85f, 1.0f, 1.0f };		// ガードの耐久値
	const Math::Color	kGuardBarLowColor	= { 1.0f, 0.4f, 0.3f, 1.0f };		// ガードの耐久値（残り少ない）
	constexpr int		kGuardBarLeft		= 236;		// ガードの耐久値のバー（GUARD の文字の右）
	constexpr int		kGuardBarWidth		= 70;
	constexpr int		kGuardBarY			= -308;
	constexpr int		kGuardBarHeight		= 4;
	constexpr float		kGuardLowRatio		= 0.35f;	// これより少ないと赤くする

	// 演出
	constexpr int		kGlowCycle			= 60;		// 満タンのゲージが明るくなったり暗くなったりする周期（フレーム）
	constexpr float		kGlowStrength		= 0.25f;	// 明るさの変化の大きさ
	constexpr int		kBarFlashFrame		= 20;		// たまった瞬間に白く光る長さ
	constexpr int		kModeFlashFrame		= 15;		// モードが変わった時に文字が大きくなる長さ
	constexpr float		kModeFlashScale		= 1.4f;		// モードが変わった瞬間の文字の大きさ

	// キーの表示（モードの文字の左に「E」、アビリティの枠の右に「F」）
	constexpr int		kKeyBadgeHalfSize	= 11;		// 枠の大きさ（半分）
	constexpr float		kKeyTextScale		= 0.75f;	// キーの文字の大きさ
	const Math::Color	kKeyBadgeColor		= { 0.85f, 0.85f, 0.9f, 0.9f };		// 枠とキーの文字
	const Math::Vector2	kModeKeyPos			= { static_cast<float>(kGageLeft - 20), kModeTextY };
	const Math::Vector2	kCommandKeyPos		= { static_cast<float>(kSlotCenterX + kSlotSpace + kSlotHalfSize + 20), static_cast<float>(kSlotCenterY) };

	// 文字の左端を x にそろえて置く（UIText は中心の位置を指定するので、幅の半分だけずらす）
	void SetTextLeft(const std::shared_ptr<UIText>& text, float x, float y)
	{
		text->SetCenterPos({ x + text->GetSize().x * 0.5f, y });
	}
}

void ATBGage::Init()
{
	m_barFlashTimers.assign(Player::kAtbBarNum, 0);
	m_isBarFull.assign(Player::kAtbBarNum, false);
	m_abilities.assign(static_cast<size_t>(AbilitySlot::Num), AbilitySlotState{});

	// モードの文字は2つとも作っておき、表示する方を切り替える
	m_operatorText = std::make_shared<UIText>();
	m_operatorText->SetText("BLADE", UIText::FontSize::Small);
	m_operatorText->SetColor(kOperatorColor);
	m_operatorText->SetScale(kModeTextScale);
	SetTextLeft(m_operatorText, kModeLeftX, kModeTextY);

	m_punisherText = std::make_shared<UIText>();
	m_punisherText->SetText("SLASH", UIText::FontSize::Small);
	m_punisherText->SetColor(kPunisherColor);
	m_punisherText->SetScale(kModeTextScale);
	SetTextLeft(m_punisherText, kModeLeftX, kModeTextY);

	// ガード中（モードの文字の下）
	m_guardText = std::make_shared<UIText>();
	m_guardText->SetText("GUARD", UIText::FontSize::Small);
	m_guardText->SetColor(kGuardColor);
	m_guardText->SetScale(kSubTextScale);
	m_guardText->SetCenterPos({ kModeLeftX + m_guardText->GetSize().x * kSubTextScale * 0.5f, kSubTextY });

	m_breakText = std::make_shared<UIText>();
	m_breakText->SetText("BREAK", UIText::FontSize::Small);
	m_breakText->SetColor(kBreakColor);
	m_breakText->SetScale(kSubTextScale);
	m_breakText->SetCenterPos({ kModeLeftX + m_breakText->GetSize().x * kSubTextScale * 0.5f, kSubTextY });

	// 「ATB」の文字（ATBゲージの上）
	m_atbLabel = std::make_shared<UIText>();
	m_atbLabel->SetText("ATB", UIText::FontSize::Small);
	m_atbLabel->SetColor(kAtbLabelColor);
	m_atbLabel->SetScale(kAtbLabelScale);
	m_atbLabel->SetCenterPos({ kGageLeft + m_atbLabel->GetSize().x * kAtbLabelScale * 0.5f, kAtbLabelY });

	// アビリティの枠の「04」「05」
	const char* const slotNames[] = { "05", "04" };
	for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
	{
		auto text = std::make_shared<UIText>();
		text->SetText(slotNames[i], UIText::FontSize::Small);
		text->SetCenterPos({ static_cast<float>(kSlotCenterX + kSlotSpace * i), static_cast<float>(kSlotCenterY - 2) });
		m_slotKeyTexts.push_back(text);
	}

	// どのキーで使えるか
	m_modeKeyBadge = CreateKeyBadge("E", kModeKeyPos);
	m_commandKeyBadge = CreateKeyBadge("F", kCommandKeyPos);
}

ATBGage::KeyBadge ATBGage::CreateKeyBadge(const std::string& key, const Math::Vector2& pos)
{
	KeyBadge badge;
	badge.text = std::make_shared<UIText>();
	badge.text->SetText(key, UIText::FontSize::Small);
	badge.text->SetScale(kKeyTextScale);
	badge.text->SetColor(kKeyBadgeColor);
	badge.text->SetCenterPos(pos);
	badge.pos = pos;
	return badge;
}

void ATBGage::DrawKeyBadge(const KeyBadge& badge)
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	int x = static_cast<int>(badge.pos.x);
	int y = static_cast<int>(badge.pos.y);
	spriteShader.DrawBox(x, y, kKeyBadgeHalfSize, kKeyBadgeHalfSize, &kBackColor, true);
	spriteShader.DrawBox(x, y, kKeyBadgeHalfSize, kKeyBadgeHalfSize, &kKeyBadgeColor, false);
	badge.text->DrawSprite();
}

void ATBGage::SetPunisherMode(bool isPunisher)
{
	// 最初の設定と、同じモードのままの時は演出しない
	if (m_isModeSet && m_isPunisher != isPunisher)
	{
		m_modeFlashTimer = kModeFlashFrame;
	}
	m_isPunisher = isPunisher;
	m_isModeSet = true;
}

void ATBGage::SetAbilityState(int index, bool canUse, float ctRatio, int cost)
{
	if (index < 0 || index >= static_cast<int>(m_abilities.size())) return;

	AbilitySlotState& slot = m_abilities[index];

	// 使えるようになった瞬間に光らせる
	if (canUse && !slot.canUse) slot.readyFlashTimer = kBarFlashFrame;

	slot.canUse = canUse;
	slot.ctRatio = std::clamp(ctRatio, 0.0f, 1.0f);
	slot.cost = cost;
}

void ATBGage::Update()
{
	m_frame++;

	// ゲージがたまった瞬間を調べて、白く光らせる
	for (int i = 0; i < Player::kAtbBarNum; ++i)
	{
		bool isFull = (m_atbValue >= static_cast<float>(i + 1));
		if (isFull && !m_isBarFull[i]) m_barFlashTimers[i] = kBarFlashFrame;
		m_isBarFull[i] = isFull;

		if (m_barFlashTimers[i] > 0) m_barFlashTimers[i]--;
	}

	for (AbilitySlotState& slot : m_abilities)
	{
		if (slot.readyFlashTimer > 0) slot.readyFlashTimer--;
	}

	// モードが変わった時は文字を大きくして、だんだん元の大きさに戻す
	float scale = 1.0f;
	if (m_modeFlashTimer > 0)
	{
		m_modeFlashTimer--;
		float t = static_cast<float>(m_modeFlashTimer) / kModeFlashFrame;
		scale = 1.0f + (kModeFlashScale - 1.0f) * t;
	}
	m_operatorText->SetScale(scale * kModeTextScale);
	m_punisherText->SetScale(scale * kModeTextScale);
}

Math::Color ATBGage::GetGlowColor(const Math::Color& baseColor) const
{
	float wave = std::sin(m_frame * DirectX::XM_2PI / kGlowCycle) * 0.5f + 0.5f;
	Math::Color color = baseColor;
	color.x = std::min(color.x + wave * kGlowStrength, 1.0f);
	color.y = std::min(color.y + wave * kGlowStrength, 1.0f);
	color.z = std::min(color.z + wave * kGlowStrength, 1.0f);
	return color;
}

void ATBGage::DrawGaugeBar(int left, int centerY, int width, int height, float ratio, const Math::Color& fillColor)
{
	// FF7R 風の斜めのゲージ（背景・たまっている部分・枠）
	float l = static_cast<float>(left), cy = static_cast<float>(centerY);
	float w = static_cast<float>(width), h = static_cast<float>(height);
	HudDraw::SlantGauge(l, cy, w, h, ratio, fillColor, kBackColor);
	HudDraw::SlantRect(l + w * 0.5f, cy, w * 0.5f, h * 0.5f, kFrameColor, HudDraw::kGaugeSlant, cy, false);
}

void ATBGage::DrawBar(int index, int left, float ratio) const
{
	ratio = std::clamp(ratio, 0.0f, 1.0f);

	// 満タンのゲージは、明るくなったり暗くなったりして「使える」ことを知らせる（まわりにうっすら光も出す）
	Math::Color color = (ratio >= 1.0f) ? GetGlowColor(kFullColor) : kFillColor;
	if (ratio >= 1.0f)
	{
		float wave = std::sin(m_frame * DirectX::XM_2PI / kGlowCycle) * 0.5f + 0.5f;
		Math::Color glow = { kFullColor.x, kFullColor.y, kFullColor.z, 0.15f + 0.2f * wave };
		HudDraw::SlantRect(left + kBarWidth * 0.5f, static_cast<float>(kGageY), kBarWidth * 0.5f + 3.0f, kBarHeight * 0.5f + 3.0f,
			glow, HudDraw::kGaugeSlant, static_cast<float>(kGageY));
	}

	// たまった瞬間は白っぽくする
	if (m_barFlashTimers[index] > 0)
	{
		float t = static_cast<float>(m_barFlashTimers[index]) / kBarFlashFrame;
		color = Math::Color::Lerp(color, Math::Color(1.0f, 1.0f, 1.0f, 1.0f), t);
	}

	DrawGaugeBar(left, kGageY, kBarWidth, kBarHeight, ratio, color);
}

void ATBGage::DrawAbilitySlot(int index) const
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	const AbilitySlotState& slot = m_abilities[index];

	int centerX = kSlotCenterX + kSlotSpace * index;
	int centerY = kSlotCenterY;

	// 背景
	spriteShader.DrawBox(centerX, centerY, kSlotHalfSize, kSlotHalfSize, &kBackColor, true);

	// CTの残り：上から暗くする（時間がたつほど暗い部分が下がっていく）
	if (slot.ctRatio > 0.0f)
	{
		int ctHalfH = static_cast<int>(kSlotHalfSize * slot.ctRatio);
		if (ctHalfH > 0)
		{
			int ctCenterY = centerY + kSlotHalfSize - ctHalfH;
			spriteShader.DrawBox(centerX, ctCenterY, kSlotHalfSize, ctHalfH, &kSlotCTColor, true);
		}
	}

	// 枠：使える時は金色で光らせ、使えるようになった瞬間は白く光らせる
	Math::Color frameColor = slot.canUse ? GetGlowColor(kSlotReadyColor) : kSlotDisableColor;
	if (slot.readyFlashTimer > 0)
	{
		float t = static_cast<float>(slot.readyFlashTimer) / kBarFlashFrame;
		frameColor = Math::Color::Lerp(frameColor, Math::Color(1.0f, 1.0f, 1.0f, 1.0f), t);
	}
	spriteShader.DrawBox(centerX, centerY, kSlotHalfSize, kSlotHalfSize, &frameColor, false);
	spriteShader.DrawBox(centerX, centerY, kSlotHalfSize - 1, kSlotHalfSize - 1, &frameColor, false);

	// 使うATBゲージの本数（枠の下に点を並べる）
	int dotY = centerY - kSlotHalfSize + 5;
	for (int i = 0; i < slot.cost; ++i)
	{
		int dotX = centerX + (i * 2 - (slot.cost - 1)) * 4;
		spriteShader.DrawBox(dotX, dotY, kSlotCostDotSize, kSlotCostDotSize, &frameColor, true);
	}
}

void ATBGage::DrawSprite()
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// パネルの左の、モードの色の斜めの線（オペレーターは水色、パニッシャーはオレンジ）
	{
		const Math::Color& mode = m_isPunisher ? kPunisherColor : kOperatorColor;
		Math::Color glow = { mode.x, mode.y, mode.z, 0.35f };
		HudDraw::SlantRect(kAccentX, HPGage::kPanelCenterY, 5.0f, HPGage::kPanelHalfH - 8.0f, glow, HudDraw::kPanelSlant, HPGage::kPanelCenterY);
		HudDraw::SlantRect(kAccentX, HPGage::kPanelCenterY, 2.0f, HPGage::kPanelHalfH - 8.0f, mode, HudDraw::kPanelSlant, HPGage::kPanelCenterY);
	}
	m_atbLabel->DrawSprite();

	// ATBゲージ（1本目は 0～1、2本目は 1～2 の分を表示する）
	for (int i = 0; i < Player::kAtbBarNum; ++i)
	{
		int left = kGageLeft + i * (kBarWidth + kBarSpace);
		DrawBar(i, left, m_atbValue - static_cast<float>(i));
	}

	// モードの固定の残り時間（モードの文字の下に、赤い線が短くなっていく）
	if (m_modeLockRatio > 0.0f)
	{
		const auto& modeText = m_isPunisher ? m_punisherText : m_operatorText;
		int width = static_cast<int>(modeText->GetSize().x * kModeTextScale * std::clamp(m_modeLockRatio, 0.0f, 1.0f));
		if (width >= 2)
		{
			spriteShader.DrawBox(static_cast<int>(kModeLeftX) + width / 2, kModeLockBarY, width / 2, kModeLockBarHeight / 2 + 1, &kLockColor, true);
		}
	}

	// ガードの耐久値（ガード中か、減っている時だけ表示する）
	if (!m_isGuardBroken && (m_isGuarding || m_guardRatio < 1.0f))
	{
		// 残り少ないと赤くする
		const Math::Color& color = (m_guardRatio < kGuardLowRatio) ? kGuardBarLowColor : kGuardBarColor;
		DrawGaugeBar(kGuardBarLeft, kGuardBarY, kGuardBarWidth, kGuardBarHeight, m_guardRatio, color);
	}

	// 文字
	if (m_isPunisher)	m_punisherText->DrawSprite();
	else				m_operatorText->DrawSprite();

	// ガードを崩されている間は BREAK（点滅）、ガード中は GUARD
	if (m_isGuardBroken)
	{
		if ((m_frame / 8) % 2 == 0) m_breakText->DrawSprite();
	}
	else if (m_isGuarding)
	{
		m_guardText->DrawSprite();
	}

	// ※どのキーで使えるか（E：モード切り替え、F：ATBコマンド）の表示は、画面をすっきりさせるために出さないようにした
}
