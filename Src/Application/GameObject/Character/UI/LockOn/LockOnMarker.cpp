#include "LockOnMarker.h"

namespace
{
	// マークの形（Triangle：頭の上に三角、Circle：体の真ん中に丸）
	constexpr LockOnMarker::Style kStyle = LockOnMarker::Style::Triangle;

	// 出てくる演出
	constexpr float		kAppearScale	= 1.8f;		// 出始めの大きさ（だんだん 1.0 へ）
	constexpr float		kAppearSpeed	= 0.15f;	// 出てくる速さ

	// だんだん薄くする
	constexpr int		kFadeStartFrame	= 30;		// 出てから、このフレームまではっきり見せる
	constexpr int		kFadeFrame		= 60;		// そこから、このフレームをかけて薄くする
	constexpr float		kFadeMinAlpha	= 0.2f;		// 一番薄い時の濃さ（ほぼ透明）

	// 三角（頭の上）
	constexpr float		kTriangleWidth	= 22.0f;	// 三角の幅
	constexpr float		kTriangleHeight	= 18.0f;	// 三角の高さ
	constexpr float		kBobSpeed		= 0.1f;		// 上下にゆれる速さ
	constexpr float		kBobSize		= 4.0f;		// 上下にゆれる幅（ピクセル）

	// 丸（体の真ん中）
	constexpr float		kRadius			= 22.0f;	// 輪の半径
	constexpr float		kRotateSpeed	= 0.05f;	// 輪の切れ目が回る速さ
	constexpr int		kCenterRadius	= 4;		// 真ん中の点の大きさ

	const Math::Color	kColor			= { 1.0f, 0.35f, 0.25f, 1.0f };	// 赤
}

LockOnMarker::Style LockOnMarker::GetStyle()
{
	return kStyle;
}

void LockOnMarker::Init()
{
	m_isVisible = false;
	m_appearRate = 0.0f;
	m_visibleFrame = 0;
}

void LockOnMarker::SetVisible(bool isVisible)
{
	// 新しく出す時は、出てくる演出・薄くするのを最初から
	if (isVisible && !m_isVisible)
	{
		m_appearRate = 0.0f;
		m_visibleFrame = 0;
	}
	m_isVisible = isVisible;
}

void LockOnMarker::Update()
{
	m_frame++;
	if (!m_isVisible) return;

	m_appearRate = std::min(m_appearRate + kAppearSpeed, 1.0f);
	m_visibleFrame++;
}

void LockOnMarker::DrawSprite()
{
	if (!m_isVisible) return;

	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);

	// 濃さ：出始めはだんだん濃く → しばらくはっきり → だんだん薄くなって、ほぼ透明のまま残る
	float fadeRate = std::clamp(static_cast<float>(m_visibleFrame - kFadeStartFrame) / kFadeFrame, 0.0f, 1.0f);
	float alpha = 1.0f + (kFadeMinAlpha - 1.0f) * fadeRate;

	Math::Color color = kColor;
	color.w *= m_appearRate * alpha;

	if (kStyle == Style::Triangle)	DrawTriangleMark(color);
	else							DrawCircleMark(color);
}

void LockOnMarker::DrawTriangleMark(const Math::Color& color) const
{
	// 下を向いた三角（先が敵の頭を指す）。出始めは大きく、その後は上下にゆれる
	float scale = kAppearScale + (1.0f - kAppearScale) * m_appearRate;
	float w = kTriangleWidth * scale * 0.5f;
	float h = kTriangleHeight * scale;
	float bob = kBobSize * std::sin(m_frame * kBobSpeed);

	Math::Vector2 tip = { m_screenPos.x, m_screenPos.y + bob };
	Math::Vector2 left = { tip.x - w, tip.y + h };
	Math::Vector2 right = { tip.x + w, tip.y + h };

	KdShaderManager::Instance().m_spriteShader.DrawTriangle(
		static_cast<int>(tip.x), static_cast<int>(tip.y),
		static_cast<int>(left.x), static_cast<int>(left.y),
		static_cast<int>(right.x), static_cast<int>(right.y),
		&color, true);
}

void LockOnMarker::DrawCircleMark(const Math::Color& color) const
{
	// 真ん中の点と、まわりの輪（輪の上に小さな点を2つ回して、動きを付ける）
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	float scale = kAppearScale + (1.0f - kAppearScale) * m_appearRate;
	float radius = kRadius * scale;
	int cx = static_cast<int>(m_screenPos.x);
	int cy = static_cast<int>(m_screenPos.y);

	spriteShader.DrawCircle(cx, cy, kCenterRadius, &color, true);
	spriteShader.DrawCircle(cx, cy, static_cast<int>(radius), &color, false);

	float angle = m_frame * kRotateSpeed;
	for (int i = 0; i < 2; ++i)
	{
		float a = angle + DirectX::XM_PI * i;
		spriteShader.DrawCircle(cx + static_cast<int>(std::cos(a) * radius), cy + static_cast<int>(std::sin(a) * radius),
			kCenterRadius, &color, true);
	}
}
