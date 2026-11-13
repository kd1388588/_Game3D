#pragma once

class Player;
class Enemy;
class UIText;

// ============================================================
// 【役割】敵の頭の上に出す、HP ゲージとブレイクゲージ
// 【どの敵に出すか】次のどれかに当てはまる敵（何体でも同時に出す）
//   ・ロックオンしている敵
//   ・最後に攻撃を当ててから 5 秒たっていない敵
//   ・近くで戦っているボス・中ボス（こちらに気づく距離にいる）
// 【頭の上に出すしくみ】敵の頭の上の 3D の位置を、カメラの行列（ビュー × 射影）で画面の位置に直す
//   射影した後の x, y を w で割ると -1～1 になる（画面の左下 ～ 右上）。それに画面の半分の大きさを掛ける
//   w が 0 以下の時は、カメラの後ろにあるので出さない
// 【見た目】・減った HP は、少し遅れて白い部分が縮む（どれだけ減ったかが分かる）
//           ・ブレイクゲージは HP ゲージの下。ブレイク中は金色に光り、残りの時間を表す
//           ・ボス・中ボスは、ザコより長いゲージ
// 【なぜ DrawBox か】四角を塗るだけなので、画像を用意しなくてよく、長さや色を自由に変えられる
// 【参考】ワールド座標からスクリーン座標への変換（KdCamera::ConvertWorldToScreenDetail と同じ計算）
// ============================================================
class EnemyGage : public KdGameObject
{
public:
	EnemyGage() {}
	~EnemyGage() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	void SetPlayer(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

private:

	// 1 体ぶんの表示の状態
	struct Entry
	{
		std::weak_ptr<Enemy>	wpEnemy;
		float					alpha = 0.0f;		// 出てくる・消える時の不透明度
		float					hpRatio = 1.0f;
		float					delayRatio = 1.0f;	// 遅れて縮む白い部分の HP の割合
		int						delayWait = 0;		// 白い部分が縮み始めるまでの残りフレーム
		float					breakRatio = 0.0f;
		bool					hasBreak = false;
		bool					isBroken = false;
		bool					isBig = false;		// ボス・中ボス（長いゲージ）
		bool					isShow = false;		// 今、出すべき敵か（false で消えきったら一覧から外す）
		Math::Vector3			headPos;			// 頭の上の位置（倒れた後も、最後の位置に出す）
	};

	// この敵のゲージを出すか
	bool ShouldShow(const std::shared_ptr<Player>& player, const std::shared_ptr<Enemy>& enemy) const;

	// 3D の位置を画面の位置に直す（カメラの後ろなら false）
	static bool WorldToScreen(const Math::Vector3& pos, Math::Vector2& out);

	// 左から伸びるゲージを 1 本描く
	static void DrawBar(int centerX, int centerY, int halfW, int halfH, float ratio, const Math::Color& color, float alpha);

	void DrawEntry(const Entry& entry);

	std::weak_ptr<Player>		m_wpPlayer;
	std::vector<Entry>			m_entries;
	int							m_frame = 0;		// 点滅に使う

	std::shared_ptr<UIText>		m_breakText = nullptr;	// 「ブレイク」
};
