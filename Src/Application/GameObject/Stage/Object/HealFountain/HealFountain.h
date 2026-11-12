#pragma once

class Player;

// ============================================================
// 【役割】中ボスの部屋の回復の泉。中ボスの部屋の敵を全員倒すと光り、近づくと 1 度だけ HP と ATB ゲージを全回復する
// 【構成】・状態：眠っている（水が暗い）→ 光っている（使える）→ 使った（暗く戻る）
//         ・Update    ：光の強さをなめらかに変え、点光源を出す。光っている時にプレイヤーが近づいたら回復する
//         ・DrawEffect：水面（寝かせた板）と、立ちのぼる光（カメラを向いた板）を加算合成で描く
// 【使い方】GameScene が作り（SetPos・SetTarget の後に Init）、中ボスの先のバリアが開いたら Activate() を呼ぶ
//          ConsumeUsed() が true を返したフレームに、「回復した」と画面に出す
// 【石の器】ステージのモデル（Tools/MakeCastleStage.py の build_fountain）。ここでは水と光だけを描く
// 画像は Tools/MakeKeyTexture.py で作成（FountainGlow.png）
// ============================================================
class HealFountain : public KdGameObject
{
public:
	HealFountain() {}
	~HealFountain() override {}

	void Init() override;
	void Update() override;
	void DrawEffect() override;

	// 泉の真ん中（床の高さ）
	void SetPos(const Math::Vector3& pos) override { m_basePos = pos; }
	Math::Vector3 GetPos() const override { return m_basePos; }

	// 回復する人（プレイヤー）
	void SetTarget(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

	// 光らせて、使えるようにする（中ボスの部屋の敵を全員倒した時）
	void Activate();
	bool IsSleeping() const { return m_state == State::Sleep; }

	// 使われた直後の 1 回だけ true を返す（メッセージを出すため）
	bool ConsumeUsed()
	{
		bool used = m_isJustUsed;
		m_isJustUsed = false;
		return used;
	}

private:

	enum class State
	{
		Sleep,		// 眠っている（まだ使えない）
		Active,		// 光っている（近づくと回復する）
		Used,		// 使った（もう使えない）
	};

	std::shared_ptr<KdSquarePolygon>	m_spWater = nullptr;	// 水面（寝かせて描く）
	std::shared_ptr<KdSquarePolygon>	m_spBeam = nullptr;		// 立ちのぼる光（カメラを向けて描く）
	std::weak_ptr<Player>				m_wpPlayer;
	Math::Vector3						m_basePos = Math::Vector3::Zero;
	State								m_state = State::Sleep;
	float								m_glowRate = 0.0f;	// 光の強さ（0～1。なめらかに変える）
	int									m_frame = 0;
	bool								m_isJustUsed = false;
};
