#include "UIBackground.h"

namespace
{
	constexpr int kScreenWidth	= 1280;
	constexpr int kScreenHeight	= 720;
}

void UIBackground::Load(const std::string& path)
{
	m_tex = std::make_shared<KdTexture>();
	if (!m_tex->Load(path))
	{
		OutputDebugStringA(("【背景の画像が読み込めませんでした】" + path + "\n").c_str());
		m_tex = nullptr;
	}
}

void UIBackground::DrawSprite()
{
	if (!m_tex) return;

	// 画面の中央（0, 0）に、画面と同じ大きさで描く
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_tex.get(), 0, 0, kScreenWidth, kScreenHeight);
}
