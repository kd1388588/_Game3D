#pragma once

// ボスの広間の入口をふさぐバリア
// ザコ敵を全員倒すまで通れない。Open() を呼ぶと少しずつ消えて通れるようになる
// 画像は Tools/MakeBarrierTexture.py で作成
class Barrier : public KdGameObject
{
public:
	Barrier() {}
	~Barrier()override {}

	void Init()override;
	void Update()override;
	void DrawEffect()override;

	// バリアを開ける（当たり判定を消して、見た目をフェードアウトさせる）
	void Open();
	bool IsOpen() const { return m_isOpen; }

private:

	std::shared_ptr<KdSquarePolygon>	m_spPolygon = nullptr;

	bool	m_isOpen = false;
	float	m_alpha = 1.0f;		// 見た目の濃さ（開けると 1 → 0）
	int		m_frame = 0;		// 点滅用のフレーム数
};
