#pragma once

class Player;
class Enemy;
class UIText;

// ============================================================
// 【役割】敵の HP ゲージと、その下のブレイクゲージ（画面の上の真ん中）
// 【どの敵を出すか】次の順に決める
//   ① ロックオンしている敵
//   ② 近くで戦っているボス・中ボス（こちらに気づく距離にいる）
//   ③ 最後に攻撃を当てた敵（5 秒たったら消す）
// 【見た目】・ボス・中ボスは長く太いゲージ。ザコは短く細いゲージ
//           ・減った HP は、少し遅れて白い部分が縮む（どれだけ減ったかが分かる）
//           ・ブレイクゲージは、ブレイクする敵（ボス）にだけ出す。ブレイク中は金色に光り、残りの時間を表す
// 【なぜ文字ではなく DrawBox か】四角を塗るだけなので、画像を用意しなくてよく、長さや色を自由に変えられる
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

	// 今、ゲージを出す敵を探す（いなければ nullptr）
	std::shared_ptr<Enemy> FindTarget() const;

	// 左から伸びるゲージを 1 本描く
	static void DrawBar(int centerX, int centerY, int halfW, int halfH, float ratio, const Math::Color& color, float alpha);

	std::weak_ptr<Player>		m_wpPlayer;
	std::weak_ptr<Enemy>		m_wpTarget;			// 今出している敵
	float						m_alpha = 0.0f;		// 出てくる・消える時の不透明度
	float						m_delayRatio = 1.0f;	// 遅れて縮む白い部分の HP の割合
	int							m_delayWait = 0;	// 白い部分が縮み始めるまでの残りフレーム
	float						m_lastHpRatio = 1.0f;
	int							m_frame = 0;		// 点滅に使う
	bool						m_isBigBar = false;	// ボス・中ボスの太いゲージか
	bool						m_hasBreak = false;	// ブレイクゲージを出すか
	float						m_hpRatio = 1.0f;
	float						m_breakRatio = 0.0f;
	bool						m_isBroken = false;

	std::shared_ptr<UIText>		m_nameText = nullptr;	// 「ボス」「中ボス」
	std::shared_ptr<UIText>		m_breakText = nullptr;	// 「ブレイク」
};
