#include "HPGage.h"
#include "../Base/HudDraw.h"
#include "../../../../UI/Text/UIText.h"
#include "../../../../../Utility/StageSettings.h"

namespace
{
	// パネル（左が濃く、右が少し明るい紺。上に明るい線、下に暗い線）
	const Math::Color	kPanelLeftColor		= { 0.02f, 0.06f, 0.13f, 0.88f };
	const Math::Color	kPanelRightColor	= { 0.04f, 0.11f, 0.24f, 0.72f };
	const Math::Color	kPanelTopLine		= { 0.55f, 0.82f, 1.0f, 0.6f };
	const Math::Color	kPanelBottomLine	= { 0.55f, 0.82f, 1.0f, 0.22f };

	// HP の数字（右端をそろえる）と、ゲージ
	constexpr float		kHpNumRightX		= 470.0f;
	constexpr float		kHpNumY				= -281.0f;
	constexpr float		kHpNumScale			= 0.8f;		// Medium（40px）を 32px に
	constexpr float		kMaxTextScale		= 0.55f;
	constexpr float		kHpBarLeft			= 330.0f;
	constexpr float		kHpBarWidth			= 175.0f;
	constexpr float		kHpBarY				= -306.0f;
	constexpr float		kHpBarHeight		= 6.0f;
	const Math::Color	kHpColor			= { 0.45f, 0.95f, 0.6f, 1.0f };		// 緑
	const Math::Color	kHpBackColor		= { 1.0f, 1.0f, 1.0f, 0.12f };
	const Math::Color	kHpLagColor			= { 1.0f, 1.0f, 1.0f, 0.55f };		// 減った分（白く残る）
	constexpr float		kLagSpeed			= 0.04f;	// 減った分が縮む速さ（1フレームに近づく割合）

	// HP が少ない時の点滅の色（暗い時 ⇔ 明るい時を行き来する）
	const Math::Color	kLowColorDark		= { 0.85f, 0.18f, 0.12f, 1.0f };	// 暗い赤
	const Math::Color	kLowColorBright		= { 1.0f, 0.55f, 0.3f, 1.0f };		// 明るいオレンジ
	const Math::Color	kLowGlowColor		= { 1.0f, 0.3f, 0.15f, 0.35f };		// まわりにうっすら出す光
}

void HPGage::Init()
{
	m_hpText = std::make_shared<UIText>();
	m_hpText->SetScale(kHpNumScale);
	m_maxText = std::make_shared<UIText>();
	m_maxText->SetScale(kMaxTextScale);
	m_maxText->SetColor({ 1.0f, 1.0f, 1.0f, 0.6f });
}

void HPGage::SetHp(int hp, int maxHp)
{
	hp = std::max(hp, 0);
	if (hp == m_hp && maxHp == m_maxHp) return;

	// 数字が変わった時だけ、文字を作り直す（右端をそろえる）
	m_hpText->SetText(std::to_string(hp), UIText::FontSize::Medium);
	m_hpText->SetCenterPos({ kHpNumRightX - m_hpText->GetSize().x * kHpNumScale * 0.5f, kHpNumY });
	if (maxHp != m_maxHp)
	{
		m_maxText->SetText("/ " + std::to_string(maxHp), UIText::FontSize::Small);
		m_maxText->SetCenterPos({ kHpNumRightX + 6.0f + m_maxText->GetSize().x * kMaxTextScale * 0.5f, kHpNumY - 5.0f });
	}
	m_hp = hp;
	m_maxHp = maxHp;
}

void HPGage::Update()
{
	m_frame++;

	// 減った分は少し遅れて縮める（回復した時はすぐ合わせる）
	if (m_lagRatio < m_hpRatio)	m_lagRatio = m_hpRatio;
	else						m_lagRatio += (m_hpRatio - m_lagRatio) * kLagSpeed;
}

void HPGage::DrawSprite()
{
	auto& sprite = KdShaderManager::Instance().m_spriteShader;
	sprite.SetMatrix(Math::Matrix::Identity);

	// ---- パネル（斜めの板） ----
	const float cx = kPanelCenterX, cy = kPanelCenterY;
	HudDraw::SlantGradient(cx, cy, kPanelHalfW, kPanelHalfH, kPanelLeftColor, kPanelRightColor, HudDraw::kPanelSlant, cy);
	HudDraw::SlantRect(cx, cy + kPanelHalfH - 0.5f, kPanelHalfW, 1.0f, kPanelTopLine, HudDraw::kPanelSlant, cy);
	HudDraw::SlantRect(cx, cy - kPanelHalfH + 0.5f, kPanelHalfW, 1.0f, kPanelBottomLine, HudDraw::kPanelSlant, cy);

	// ---- HP ゲージ ----
	const StageSettings::LowHpScreen& low = StageSettings::Instance().GetLowHpScreen();
	bool isLow = (m_hpRatio <= low.blinkRate);
	float pulse = 0.5f + 0.5f * std::sin(m_frame * low.blinkSpeed);

	Math::Color barColor = isLow ? Math::Color::Lerp(kLowColorDark, kLowColorBright, pulse) : kHpColor;
	if (isLow && m_hpRatio > 0.0f)
	{
		// まわりにうっすら光を出す
		Math::Color glow = kLowGlowColor;
		glow.w *= pulse;
		float w = kHpBarWidth * m_hpRatio;
		HudDraw::SlantRect(kHpBarLeft + w * 0.5f, kHpBarY, w * 0.5f + 3.0f, kHpBarHeight * 0.5f + 3.0f, glow, HudDraw::kGaugeSlant, kHpBarY);
	}
	HudDraw::SlantGauge(kHpBarLeft, kHpBarY, kHpBarWidth, kHpBarHeight, m_lagRatio, kHpLagColor, kHpBackColor);
	HudDraw::SlantGauge(kHpBarLeft, kHpBarY, kHpBarWidth, kHpBarHeight, m_hpRatio, barColor, { 0, 0, 0, 0 });

	// ---- HP の数字（少ない時は、ゲージと同じ色で点滅） ----
	if (m_hp >= 0)
	{
		m_hpText->SetColor(isLow ? HudDraw::Brighten(barColor, 0.15f) : Math::Color(1.0f, 1.0f, 1.0f, 1.0f));
		m_hpText->DrawSprite();
		m_maxText->DrawSprite();
	}
	sprite.SetMatrix(Math::Matrix::Identity);
}

void HPGage::Release()
{
	m_hpText = nullptr;
	m_maxText = nullptr;
}
