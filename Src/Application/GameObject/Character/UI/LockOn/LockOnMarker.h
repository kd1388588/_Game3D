#pragma once

// ロックオンのマーク
// 形は2種類（kStyle で切り替え）：
//   Triangle：敵の頭の上に、下を向いた三角（上下にゆらゆら）
//   Circle  ：敵の体の真ん中に、丸（回る輪）
// 出た時ははっきり見え、だんだん薄くなって、ほぼ透明のまま残る（画面のじゃまにならないように）
class LockOnMarker : public KdGameObject
{
public:
	enum class Style
	{
		Triangle,	// 頭の上に三角
		Circle,		// 体の真ん中に丸
	};

	// 今の形（Player がマークを出す高さを決めるのに使う）
	static Style GetStyle();

	LockOnMarker() {}
	~LockOnMarker() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	// 表示するか（ロックオンしていない・敵がカメラの後ろの時は false）
	void SetVisible(bool isVisible);

	// もう一度はっきり出す（注目した時など）
	void Flash() { m_visibleFrame = 0; }

	// 画面の座標（画面の中央が原点、上が＋Y）
	void SetScreenPos(const Math::Vector2& pos) { m_screenPos = pos; }

private:

	void DrawTriangleMark(const Math::Color& color) const;
	void DrawCircleMark(const Math::Color& color) const;

	bool			m_isVisible = false;
	Math::Vector2	m_screenPos = Math::Vector2::Zero;
	float			m_appearRate = 0.0f;	// 出てきた具合（0～1。出始めは大きく、だんだん縮む）
	int				m_visibleFrame = 0;		// 出てからのフレーム数（だんだん薄くするのに使う）
	int				m_frame = 0;
};
