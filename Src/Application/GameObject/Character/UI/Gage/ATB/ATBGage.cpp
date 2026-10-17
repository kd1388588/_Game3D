#include "ATBGage.h"
#include "../../../Player/Player.h"
#include "../../../../UI/Text/UIText.h"

namespace
{
	// 位置・大きさ（HPゲージ（左端 -500、高さ -300、幅 200）のまわりに並べる）
	constexpr int		kGageLeft			= -500;		// ATBゲージ1本目の左端
	constexpr int		kGageY				= -322;		// ATBゲージの中心の高さ
	constexpr int		kBarWidth			= 96;		// ATBゲージ1本の幅
	constexpr int		kBarHeight			= 10;		// ATBゲージ1本の高さ
	constexpr int		kBarSpace			= 8;		// ゲージとゲージの間
	constexpr float		kModeTextY			= -268.0f;	// モード表示の高さ（HPゲージの上）
	constexpr int		kModeLockBarY		= -283;		// モードの固定の残り時間のバーの高さ
	constexpr int		kModeLockBarHeight	= 2;

	// アビリティの枠（HPゲージ・ATBゲージの右）
	constexpr int		kSlotCenterX		= -270;		// 1つ目の枠の中心
	constexpr int		kSlotSpace			= 42;		// 枠と枠の間隔
	constexpr int		kSlotCenterY		= -311;
	constexpr int		kSlotHalfSize		= 17;
	constexpr int		kSlotCostDotSize	= 2;		// 使うゲージの本数を表す点の大きさ（半分）

	// WPゲージ（ATBゲージの下）
	constexpr int		kWpY				= -339;
	constexpr int		kWpWidth			= 200;
	constexpr int		kWpHeight			= 6;

	// 色
	const Math::Color	kBackColor			= { 0.05f, 0.05f, 0.1f, 0.75f };	// 背景
	const Math::Color	kFrameColor			= { 0.6f, 0.7f, 0.85f, 0.9f };		// 枠
	const Math::Color	kFillColor			= { 0.2f, 0.6f, 1.0f, 1.0f };		// たまっている途中
	const Math::Color	kFullColor			= { 1.0f, 0.85f, 0.3f, 1.0f };		// 満タン（アビリティが使える）
	const Math::Color	kOperatorColor		= { 0.45f, 0.8f, 1.0f, 1.0f };		// OPERATOR の文字
	const Math::Color	kPunisherColor		= { 1.0f, 0.6f, 0.2f, 1.0f };		// PUNISHER の文字
	const Math::Color	kLockColor			= { 0.9f, 0.3f, 0.3f, 1.0f };		// モードの固定の残り時間
	const Math::Color	kSlotReadyColor		= { 1.0f, 0.85f, 0.3f, 1.0f };		// 使えるアビリティの枠
	const Math::Color	kSlotDisableColor	= { 0.4f, 0.4f, 0.45f, 0.9f };		// 使えないアビリティの枠
	const Math::Color	kSlotCTColor		= { 0.0f, 0.0f, 0.0f, 0.65f };		// CTの残り（上から暗くする）
	const Math::Color	kWpColor			= { 0.75f, 0.35f, 1.0f, 1.0f };		// WPゲージ
	const Math::Color	kWpFullColor		= { 1.0f, 0.5f, 1.0f, 1.0f };		// WPゲージ（満タン・覚醒中）
	const Math::Color	kGuardColor			= { 0.85f, 0.95f, 1.0f, 1.0f };		// GUARD の文字

	// 演出
	constexpr int		kGlowCycle			= 60;		// 満タンのゲージが明るくなったり暗くなったりする周期（フレーム）
	constexpr float		kGlowStrength		= 0.25f;	// 明るさの変化の大きさ
	constexpr int		kBarFlashFrame		= 20;		// たまった瞬間に白く光る長さ
	constexpr int		kModeFlashFrame		= 15;		// モードが変わった時に文字が大きくなる長さ
	constexpr float		kModeFlashScale		= 1.4f;		// モードが変わった瞬間の文字の大きさ

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
	m_operatorText->SetText("OPERATOR", UIText::FontSize::Small);
	m_operatorText->SetColor(kOperatorColor);
	SetTextLeft(m_operatorText, static_cast<float>(kGageLeft), kModeTextY);

	m_punisherText = std::make_shared<UIText>();
	m_punisherText->SetText("PUNISHER", UIText::FontSize::Small);
	m_punisherText->SetColor(kPunisherColor);
	SetTextLeft(m_punisherText, static_cast<float>(kGageLeft), kModeTextY);

	// ガード中（モードの文字の右）
	m_guardText = std::make_shared<UIText>();
	m_guardText->SetText("GUARD", UIText::FontSize::Small);
	m_guardText->SetColor(kGuardColor);
	SetTextLeft(m_guardText, static_cast<float>(kGageLeft + 140), kModeTextY);

	// WPゲージの文字（ゲージの左）と、満タンの時の案内（ゲージの右）
	m_wpText = std::make_shared<UIText>();
	m_wpText->SetText("WP", UIText::FontSize::Small);
	m_wpText->SetColor(kWpColor);
	m_wpText->SetCenterPos({ static_cast<float>(kGageLeft) - m_wpText->GetSize().x * 0.5f - 6.0f, static_cast<float>(kWpY) });

	m_wpReadyText = std::make_shared<UIText>();
	m_wpReadyText->SetText("G：覚醒", UIText::FontSize::Small);
	m_wpReadyText->SetColor(kWpFullColor);
	SetTextLeft(m_wpReadyText, static_cast<float>(kGageLeft + kWpWidth + 8), static_cast<float>(kWpY));

	// アビリティの枠の「1」「2」
	for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
	{
		auto text = std::make_shared<UIText>();
		text->SetText(std::to_string(i + 1), UIText::FontSize::Small);
		text->SetCenterPos({ static_cast<float>(kSlotCenterX + kSlotSpace * i), static_cast<float>(kSlotCenterY - 2) });
		m_slotKeyTexts.push_back(text);
	}
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
	m_operatorText->SetScale(scale);
	m_punisherText->SetScale(scale);
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

void ATBGage::DrawBar(int index, int left, float ratio) const
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	constexpr int halfW = kBarWidth / 2;
	constexpr int halfH = kBarHeight / 2;
	int centerX = left + halfW;

	// 背景
	spriteShader.DrawBox(centerX, kGageY, halfW, halfH, &kBackColor, true);

	// たまっている部分（左から伸びる）
	ratio = std::clamp(ratio, 0.0f, 1.0f);
	int fillHalfW = static_cast<int>(kBarWidth * ratio * 0.5f);
	if (fillHalfW > 0)
	{
		// 満タンのゲージは、明るくなったり暗くなったりして「使える」ことを知らせる
		Math::Color color = (ratio >= 1.0f) ? GetGlowColor(kFullColor) : kFillColor;

		// たまった瞬間は白っぽくする
		if (m_barFlashTimers[index] > 0)
		{
			float t = static_cast<float>(m_barFlashTimers[index]) / kBarFlashFrame;
			color = Math::Color::Lerp(color, Math::Color(1.0f, 1.0f, 1.0f, 1.0f), t);
		}

		spriteShader.DrawBox(left + fillHalfW, kGageY, fillHalfW, halfH, &color, true);
	}

	// 枠
	spriteShader.DrawBox(centerX, kGageY, halfW, halfH, &kFrameColor, false);
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

void ATBGage::DrawWpBar() const
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	constexpr int halfW = kWpWidth / 2;
	constexpr int halfH = kWpHeight / 2;
	int centerX = kGageLeft + halfW;

	spriteShader.DrawBox(centerX, kWpY, halfW, halfH, &kBackColor, true);

	float ratio = std::clamp(m_wpRatio, 0.0f, 1.0f);
	int fillHalfW = static_cast<int>(kWpWidth * ratio * 0.5f);
	if (fillHalfW > 0)
	{
		bool isFull = (ratio >= 1.0f) || m_isAwakening;
		Math::Color color = isFull ? GetGlowColor(kWpFullColor) : kWpColor;
		spriteShader.DrawBox(kGageLeft + fillHalfW, kWpY, fillHalfW, halfH, &color, true);
	}

	spriteShader.DrawBox(centerX, kWpY, halfW, halfH, &kFrameColor, false);
}

void ATBGage::DrawSprite()
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// ATBゲージ（1本目は 0～1、2本目は 1～2 の分を表示する）
	for (int i = 0; i < Player::kAtbBarNum; ++i)
	{
		int left = kGageLeft + i * (kBarWidth + kBarSpace);
		DrawBar(i, left, m_atbValue - static_cast<float>(i));
	}

	// アビリティの枠
	for (int i = 0; i < static_cast<int>(m_abilities.size()); ++i)
	{
		DrawAbilitySlot(i);
	}

	// WPゲージ
	DrawWpBar();

	// モードの固定の残り時間（モードの文字の下に、赤い線が短くなっていく）
	if (m_modeLockRatio > 0.0f)
	{
		const auto& modeText = m_isPunisher ? m_punisherText : m_operatorText;
		int width = static_cast<int>(modeText->GetSize().x * std::clamp(m_modeLockRatio, 0.0f, 1.0f));
		if (width >= 2)
		{
			spriteShader.DrawBox(kGageLeft + width / 2, kModeLockBarY, width / 2, kModeLockBarHeight / 2 + 1, &kLockColor, true);
		}
	}

	// 文字
	if (m_isPunisher)	m_punisherText->DrawSprite();
	else				m_operatorText->DrawSprite();

	if (m_isGuarding) m_guardText->DrawSprite();

	m_wpText->DrawSprite();
	if (m_wpRatio >= 1.0f && !m_isAwakening) m_wpReadyText->DrawSprite();

	for (const auto& text : m_slotKeyTexts)
	{
		text->DrawSprite();
	}
}
