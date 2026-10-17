#pragma once

// ロックオンのマーク（ロックオン中の敵の胸のあたりに、回る4つの三角と丸を出す）
class LockOnMarker : public KdGameObject
{
public:
	LockOnMarker() {}
	~LockOnMarker() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	// 表示するか（ロックオンしていない・敵がカメラの後ろの時は false）
	void SetVisible(bool isVisible);

	// 画面の座標（画面の中央が原点、上が＋Y）
	void SetScreenPos(const Math::Vector2& pos) { m_screenPos = pos; }

private:

	bool			m_isVisible = false;
	Math::Vector2	m_screenPos = Math::Vector2::Zero;
	float			m_appearRate = 0.0f;	// 出てきた具合（0～1。出始めは大きく、だんだん縮む）
	int				m_frame = 0;
};
