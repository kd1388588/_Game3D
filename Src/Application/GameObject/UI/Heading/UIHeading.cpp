#include "UIHeading.h"

void UIHeading::Load(const std::string& path)
{
	m_tex = std::make_shared<KdTexture>();
	if (!m_tex->Load(path))
	{
		OutputDebugStringA(("【見出しの画像が読み込めませんでした】" + path + "\n").c_str());
		m_tex = nullptr;
	}
}

void UIHeading::DrawSprite()
{
	if (!m_tex) return;

	// 画像と同じ大きさで、中心に描く（拡大しないので、文字がぼやけない）
	const auto& info = m_tex->GetInfo();
	KdShaderManager::Instance().m_spriteShader.DrawTex(m_tex.get(),
		static_cast<int>(m_centerPos.x), static_cast<int>(m_centerPos.y),
		static_cast<int>(info.Width), static_cast<int>(info.Height), nullptr, &m_color);
}
