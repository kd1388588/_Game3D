#include "HPGage.h"

void HPGage::Init()
{
	m_gage.Load("Asset/Textures/UI/Gage/HPGage.png");
	m_gageBase.Load("Asset/Textures/UI/Gage/BackGage.png");
}

void HPGage::Update()
{
	m_gageWidth = (int)(200.0f * m_hpRatio);
	m_mWorld = Math::Matrix::CreateTranslation(-500.0f, -300.0f, 0.0f);
}

void HPGage::DrawSprite()
{
	Math::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	Math::Vector2 pivot = { 0.0f, 0.5f };

	KdShaderManager::Instance().m_spriteShader.SetMatrix(m_mWorld);

	KdShaderManager::Instance().m_spriteShader.DrawTex(
		&m_gageBase, 0, 0, 200, 20, nullptr, &color, pivot
	);

	KdShaderManager::Instance().m_spriteShader.DrawTex(
		&m_gage, 0, 0, m_gageWidth, 20, nullptr, &color, pivot
	);

	KdShaderManager::Instance().m_spriteShader.SetMatrix(Math::Matrix::Identity);
}

void HPGage::Release()
{
	m_gage.Release();
	m_gageBase.Release();
}