#include "LockOnMarker.h"

namespace
{
	constexpr float		kRadius			= 26.0f;	// 三角を並べる円の半径
	constexpr float		kAppearScale	= 1.8f;		// 出始めの大きさ（だんだん 1.0 へ）
	constexpr float		kAppearSpeed	= 0.15f;	// 出てくる速さ
	constexpr float		kRotateSpeed	= 0.03f;	// 回る速さ（ラジアン/フレーム）
	constexpr float		kPulseSpeed		= 0.12f;	// 大きさの揺れの速さ
	constexpr float		kPulseSize		= 0.08f;	// 大きさの揺れ幅
	constexpr float		kTriangleSize	= 9.0f;		// 三角の大きさ
	constexpr int		kCenterRadius	= 4;		// 真ん中の丸の大きさ
	const Math::Color	kColor			= { 1.0f, 0.35f, 0.25f, 1.0f };	// 赤
}

void LockOnMarker::Init()
{
	m_isVisible = false;
	m_appearRate = 0.0f;
}

void LockOnMarker::SetVisible(bool isVisible)
{
	// 新しく出す時は、出てくる演出を最初から
	if (isVisible && !m_isVisible) m_appearRate = 0.0f;
	m_isVisible = isVisible;
}

void LockOnMarker::Update()
{
	m_frame++;
	if (m_isVisible) m_appearRate = std::min(m_appearRate + kAppearSpeed, 1.0f);
}

void LockOnMarker::DrawSprite()
{
	if (!m_isVisible) return;

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;
	spriteShader.SetMatrix(Math::Matrix::Identity);

	// 出始めは大きく薄く、だんだん小さく濃くなる。出た後は少し脈打つ
	float appearScale = kAppearScale + (1.0f - kAppearScale) * m_appearRate;
	float pulse = 1.0f + kPulseSize * std::sin(m_frame * kPulseSpeed);
	float radius = kRadius * appearScale * pulse;

	Math::Color color = kColor;
	color.w *= m_appearRate;

	// 円の上に、中心を向いた三角を4つ並べて回す
	float baseAngle = m_frame * kRotateSpeed;
	for (int i = 0; i < 4; ++i)
	{
		float angle = baseAngle + DirectX::XM_PIDIV2 * i;
		Math::Vector2 dir = { std::cos(angle), std::sin(angle) };
		Math::Vector2 side = { -dir.y, dir.x };

		Math::Vector2 tip = m_screenPos + dir * radius;						// 中心側の先
		Math::Vector2 back = m_screenPos + dir * (radius + kTriangleSize);	// 外側
		Math::Vector2 left = back + side * (kTriangleSize * 0.6f);
		Math::Vector2 right = back - side * (kTriangleSize * 0.6f);

		spriteShader.DrawTriangle(
			static_cast<int>(tip.x), static_cast<int>(tip.y),
			static_cast<int>(left.x), static_cast<int>(left.y),
			static_cast<int>(right.x), static_cast<int>(right.y),
			&color, true);
	}

	// 真ん中の丸と、まわりの円
	spriteShader.DrawCircle(static_cast<int>(m_screenPos.x), static_cast<int>(m_screenPos.y), kCenterRadius, &color, true);
	spriteShader.DrawCircle(static_cast<int>(m_screenPos.x), static_cast<int>(m_screenPos.y), static_cast<int>(radius * 0.75f), &color, false);
}
