#pragma once

// 部屋の入口をふさぐバリア（門楼の入口・大聖堂の入口）
// その部屋の手前の敵を全員倒すまで通れない。Open() を呼ぶと少しずつ消えて通れるようになる
// 置く場所と大きさは、Init の前に SetArea で決める
// 画像は Tools/MakeBarrierTexture.py で作成
class Barrier : public KdGameObject
{
public:
	Barrier() {}
	~Barrier()override {}

	void Init()override;
	void Update()override;
	void DrawEffect()override;

	// 置く場所（下の真ん中）と大きさ（幅・高さ）。Init の前に呼ぶ
	void SetArea(const Math::Vector3& pos, const Math::Vector2& size) { m_pos = pos; m_size = size; }

	// バリアを開ける（当たり判定を消して、見た目をフェードアウトさせる）
	void Open();
	bool IsOpen() const { return m_isOpen; }

private:

	std::shared_ptr<KdSquarePolygon>	m_spPolygon = nullptr;

	Math::Vector3	m_pos = { 0.0f, 0.0f, 0.0f };
	Math::Vector2	m_size = { 10.0f, 10.0f };

	bool	m_isOpen = false;
	float	m_alpha = 1.0f;		// 見た目の濃さ（開けると 1 → 0）
	int		m_frame = 0;		// 点滅用のフレーム数
};
