#include "HPGage.h"

namespace
{
	constexpr int		kGageWidth	= 200;
	constexpr int		kGageHeight	= 20;
	const Math::Vector3	kGagePos	= { -500.0f, -300.0f, 0.0f };
}

void HPGage::Init()
{
	m_gage.Load("Asset/Textures/UI/Gage/HPGage.png");
	m_gageBase.Load("Asset/Textures/UI/Gage/BackGage.png");
}

void HPGage::Update()
{
	m_gageWidth = static_cast<int>(kGageWidth * m_hpRatio);
	m_mWorld = Math::Matrix::CreateTranslation(kGagePos);
}

void HPGage::DrawSprite()
{
	Math::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Vector2 pivot = { 0.0f, 0.5f };

	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	spriteShader.SetMatrix(m_mWorld);
	spriteShader.DrawTex(&m_gageBase, 0, 0, kGageWidth, kGageHeight, nullptr, &color, pivot);
	spriteShader.DrawTex(&m_gage, 0, 0, m_gageWidth, kGageHeight, nullptr, &color, pivot);
	spriteShader.SetMatrix(Math::Matrix::Identity);
}

void HPGage::Release()
{
	m_gage.Release();
	m_gageBase.Release();
}