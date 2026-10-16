#include "ATBGage.h"
#include "../../../Player/Player.h"
#include "../../../../UI/Text/UIText.h"

namespace
{
	// 位置・大きさ（HPゲージ（左端 -500、高さ -300、幅 200）の下に並べる）
	constexpr int		kGageLeft			= -500;		// 1本目の左端
	constexpr int		kGageY				= -322;		// ゲージの中心の高さ
	constexpr int		kBarWidth			= 96;		// 1本の幅
	constexpr int		kBarHeight			= 10;		// 1本の高さ
	constexpr int		kBarSpace			= 8;		// ゲージとゲージの間
	constexpr float		kModeTextY			= -268.0f;	// モード表示の高さ（HPゲージの上）

	// 色
	const Math::Color	kBackColor			= { 0.05f, 0.05f, 0.1f, 0.75f };	// 背景
	const Math::Color	kFrameColor			= { 0.6f, 0.7f, 0.85f, 0.9f };		// 枠
	const Math::Color	kFillColor			= { 0.2f, 0.6f, 1.0f, 1.0f };		// たまっている途中
	const Math::Color	kFullColor			= { 1.0f, 0.85f, 0.3f, 1.0f };		// 満タン（アビリティが使える）
	const Math::Color	kOperatorColor		= { 0.45f, 0.8f, 1.0f, 1.0f };		// OPERATOR の文字
	const Math::Color	kPunisherColor		= { 1.0f, 0.6f, 0.2f, 1.0f };		// PUNISHER の文字

	// 演出
	constexpr int		kGlowCycle			= 60;		// 満タンのゲージが明るくなったり暗くなったりする周期（フレーム）
	constexpr float		kGlowStrength		= 0.25f;	// 明るさの変化の大きさ
	constexpr int		kBarFlashFrame		= 20;		// たまった瞬間に白く光る長さ
	constexpr int		kModeFlashFrame		= 15;		// モードが変わった時に文字が大きくなる長さ
	constexpr float		kModeFlashScale		= 1.4f;		// モードが変わった瞬間の文字の大きさ
}

void ATBGage::Init()
{
	m_barFlashTimers.assign(Player::kAtbBarNum, 0);
	m_isBarFull.assign(Player::kAtbBarNum, false);

	// モードの文字は2つとも作っておき、表示する方を切り替える
	m_operatorText = std::make_shared<UIText>();
	m_operatorText->SetText("OPERATOR", UIText::FontSize::Small);
	m_operatorText->SetColor(kOperatorColor);

	m_punisherText = std::make_shared<UIText>();
	m_punisherText->SetText("PUNISHER", UIText::FontSize::Small);
	m_punisherText->SetColor(kPunisherColor);

	// 文字の左端をゲージの左端にそろえる（UIText は中心の位置を指定するので、幅の半分だけずらす）
	for (const auto& text : { m_operatorText, m_punisherText })
	{
		text->SetCenterPos({ kGageLeft + text->GetSize().x * 0.5f, kModeTextY });
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
		Math::Color color = kFillColor;
		if (ratio >= 1.0f)
		{
			// 満タンのゲージは、明るくなったり暗くなったりして「使える」ことを知らせる
			float wave = std::sin(m_frame * DirectX::XM_2PI / kGlowCycle) * 0.5f + 0.5f;
			color = kFullColor;
			color.x = std::min(color.x + wave * kGlowStrength, 1.0f);
			color.y = std::min(color.y + wave * kGlowStrength, 1.0f);
			color.z = std::min(color.z + wave * kGlowStrength, 1.0f);
		}

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

void ATBGage::DrawSprite()
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// ゲージ（1本目は 0～1、2本目は 1～2 の分を表示する）
	for (int i = 0; i < Player::kAtbBarNum; ++i)
	{
		int left = kGageLeft + i * (kBarWidth + kBarSpace);
		DrawBar(i, left, m_atbValue - static_cast<float>(i));
	}

	// 今のモード
	if (m_isPunisher)	m_punisherText->DrawSprite();
	else				m_operatorText->DrawSprite();
}
