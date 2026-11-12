#pragma once

// 部屋の入口をふさぐバリア（中ボスの先の通路・右の部屋の出口・ボスの部屋の鍵の扉）
// その部屋の敵を全員倒す（鍵の扉は、鍵を持って近づく）まで通れない。Open() を呼ぶと少しずつ消えて通れるようになる
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

	// 置く場所（下の真ん中）と大きさ（幅・高さ）、向き（度。0 で板が X の向きに広がる＝奥へ進む通路をふさぐ。
	// 90 で Z の向きに広がる＝横へ進む通路をふさぐ）。Init の前に呼ぶ
	void SetArea(const Math::Vector3& pos, const Math::Vector2& size, float yawDeg = 0.0f) { m_pos = pos; m_size = size; m_yawDeg = yawDeg; }

	// 光の色（画像の色にかける）。鍵の扉は金色にして、普通のバリアと見分けられるようにする。Init の前でも後でもよい
	void SetColor(const Math::Vector3& color) { m_color = color; }

	// 置いた場所（下の真ん中）
	const Math::Vector3& GetBasePos() const { return m_pos; }

	// バリアを開ける（当たり判定を消して、見た目をフェードアウトさせる）
	void Open();
	bool IsOpen() const { return m_isOpen; }

private:

	std::shared_ptr<KdSquarePolygon>	m_spPolygon = nullptr;

	Math::Vector3	m_pos = { 0.0f, 0.0f, 0.0f };
	Math::Vector2	m_size = { 10.0f, 10.0f };
	float			m_yawDeg = 0.0f;
	Math::Vector3	m_color = { 0.55f, 0.6f, 1.0f };	// 光の色（青白い）

	bool	m_isOpen = false;
	float	m_alpha = 1.0f;		// 見た目の濃さ（開けると 1 → 0）
	int		m_frame = 0;		// 点滅用のフレーム数
};
