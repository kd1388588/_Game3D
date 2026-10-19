#pragma once

// 画面いっぱいに1枚の画像を表示する（リザルト画面などの背景）
class UIBackground : public KdGameObject
{
public:
	UIBackground() {}
	~UIBackground() override {}

	// 画像を読み込む（無ければ何も表示しない）
	void Load(const std::string& path);

	void DrawSprite() override;

private:
	std::shared_ptr<KdTexture>	m_tex = nullptr;
};
