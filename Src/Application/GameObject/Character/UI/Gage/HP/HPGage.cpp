#include "HPGage.h"
#include "../../../../../Utility/StageSettings.h"

namespace
{
	constexpr int		kGageWidth	= 200;
	constexpr int		kGageHeight	= 20;
	const Math::Vector3	kGagePos	= { -500.0f, -300.0f, 0.0f };
	constexpr int		kFrameWidth	= 2;	// 枠の太さ（ゲージの中身は枠の内側に描く。画像は Tools/MakeUITexture.py で作成）

	// HP が少ない時の点滅の色（暗い時 ⇔ 明るい時を行き来する）
	const Math::Color	kLowColorDark	= { 0.85f, 0.18f, 0.12f, 1.0f };	// 暗い赤
	const Math::Color	kLowColorBright	= { 1.0f, 0.55f, 0.3f, 1.0f };		// 明るいオレンジ
	const Math::Color	kLowGlowColor	= { 1.0f, 0.3f, 0.15f, 0.35f };		// まわりにうっすら出す光
}

void HPGage::Init()
{
	m_gage.Load("Asset/Textures/UI/Gage/HPGage.png");
	m_gageBase.Load("Asset/Textures/UI/Gage/BackGage.png");
}

void HPGage::Update()
{
	m_frame++;
	m_gageWidth = static_cast<int>((kGageWidth - kFrameWidth * 2) * m_hpRatio);
	m_mWorld = Math::Matrix::CreateTranslation(kGagePos);
}

void HPGage::DrawSprite()
{
	Math::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Vector2 pivot = { 0.0f, 0.5f };

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	spriteShader.SetMatrix(m_mWorld);
	spriteShader.DrawTex(&m_gageBase, 0, 0, kGageWidth, kGageHeight, nullptr, &color, pivot);

	// HP が少ない時は、ゲージを赤～オレンジでゆっくり点滅させる（設定はステージエディタの「画面」タブ）
	const StageSettings::LowHpScreen& low = StageSettings::Instance().GetLowHpScreen();
	if (m_hpRatio <= low.blinkRate && m_gageWidth > 0)
	{
		float pulse = 0.5f + 0.5f * std::sin(m_frame * low.blinkSpeed);
		Math::Color barColor = Math::Color::Lerp(kLowColorDark, kLowColorBright, pulse);
		Math::Color glowColor = kLowGlowColor;
		glowColor.w *= pulse;

		spriteShader.SetMatrix(Math::Matrix::Identity);
		int halfW = m_gageWidth / 2;
		int halfH = (kGageHeight - kFrameWidth * 2) / 2;
		int centerX = static_cast<int>(kGagePos.x) + kFrameWidth + halfW;
		int centerY = static_cast<int>(kGagePos.y);
		spriteShader.DrawBox(centerX, centerY, halfW + 3, halfH + 3, &glowColor, true);	// まわりの光
		spriteShader.DrawBox(centerX, centerY, halfW, halfH, &barColor, true);
		return;
	}

	spriteShader.DrawTex(&m_gage, kFrameWidth, 0, m_gageWidth, kGageHeight - kFrameWidth * 2, nullptr, &color, pivot);
	spriteShader.SetMatrix(Math::Matrix::Identity);
}

void HPGage::Release()
{
	m_gage.Release();
	m_gageBase.Release();
}