#pragma once

// 画像にした見出しの文字（GAME CLEAR / GAME OVER など）を表示するUI
// 画像は Tools/MakeHeadingImages.py で、白い文字（透明度つき）として作ってある。色はここでかける
// ※見出しのフォント（Cinzel）は、Windows の文字の描画だと「M」が欠けるので、UIText ではなく画像で出す
class UIHeading : public KdGameObject
{
public:
	UIHeading() {}
	~UIHeading() override {}

	// 画像を読み込む（無ければ何も表示しない）
	void Load(const std::string& path);

	// 中心の位置（画面中央が原点、上が+Y）と色
	void SetCenterPos(const Math::Vector2& pos) { m_centerPos = pos; }
	void SetColor(const Math::Color& color) { m_color = color; }

	void DrawSprite() override;

private:
	std::shared_ptr<KdTexture>	m_tex = nullptr;
	Math::Vector2				m_centerPos = Math::Vector2::Zero;
	Math::Color					m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
};
