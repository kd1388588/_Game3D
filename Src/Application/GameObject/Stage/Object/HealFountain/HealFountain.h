#pragma once

class Player;

// ============================================================
// 【役割】中ボスの部屋の右の、泉の小部屋の回復の泉
//         中ボスの部屋の敵を全員倒すと光り、近づくと 1 度だけ HP と ATB ゲージを全回復する
// 【構成】・状態：眠っている（ふつうの泉）→ 光っている（使える）→ 使った（ふつうの泉に戻る）
//         ・DrawLit   ：水面（波の模様の板を寝かせて、松明の光で照らされるように描く。丸く切り抜く）
//         ・DrawEffect：水面のきらめき・上の皿から流れ落ちる水・落ちた所の波紋・光っている時の光の柱（加算合成）
//         ・Update    ：水の流れ・波紋・光の強さを動かし、光っている時にプレイヤーが近づいたら回復する
// 【水らしく見せる工夫】
//   ・水面：波の模様の画像を、ゆっくり回して揺らぐように見せる（丸く切り抜くので、回しても形は変わらない）
//           丸の外はアルファ 0 にして、シェーダーのアルファテスト（0.55 未満は描かない）で切り抜く
//   ・流れ落ちる水：縦の筋の画像の切り出し位置（UV）を毎フレーム下へずらすと、模様が流れて見える
//           （サンプラーが Wrap なので、0～1 をはみ出した所は繰り返して貼られる）
//   ・波紋：水が落ちる所に輪を出し、広げながら薄くする
// 【使い方】GameScene が作り（SetPos・SetTarget の後に Init）、中ボスの部屋の敵を全員倒したら Activate() を呼ぶ
//          ConsumeUsed() が true を返したフレームに、「回復した」と画面に出す
// 【石の器】ステージのモデル（Tools/MakeCastleStage.py の build_fountain）。ここでは水と光だけを描く
// 画像は Tools/MakeKeyTexture.py で作成（FountainWater・Shine・Stream・Ring・Glow）
// ============================================================
class HealFountain : public KdGameObject
{
public:
	HealFountain() {}
	~HealFountain() override {}

	void Init() override;
	void Update() override;
	void DrawLit() override;
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

	// 水が落ちた所に広がる波紋 1 つ
	struct Ripple
	{
		Math::Vector3	pos;		// 水面の上の位置
		int				age = 0;	// 出てからのフレーム数
	};

	// 板を水面と同じ向き（床と平行）に寝かせる行列
	Math::Matrix CreateFlatMatrix(float size, float rotY, const Math::Vector3& pos) const;

	// 光る物を描く準備（加算・両面・深度を書かない）と、元に戻す
	void BeginAdditive() const;
	void EndAdditive() const;

	std::shared_ptr<KdSquarePolygon>	m_spWater = nullptr;	// 水面（光で照らされる）
	std::shared_ptr<KdSquarePolygon>	m_spShine = nullptr;	// 水面のきらめき
	std::shared_ptr<KdSquarePolygon>	m_spStream = nullptr;	// 流れ落ちる水
	std::shared_ptr<KdSquarePolygon>	m_spRing = nullptr;		// 波紋
	std::shared_ptr<KdSquarePolygon>	m_spBeam = nullptr;		// 光っている時の光の柱

	std::weak_ptr<Player>				m_wpPlayer;
	Math::Vector3						m_basePos = Math::Vector3::Zero;
	State								m_state = State::Sleep;
	float								m_glowRate = 0.0f;	// 光の強さ（0～1。なめらかに変える）
	float								m_streamScroll = 0.0f;	// 流れ落ちる水の模様をずらした量
	std::vector<Ripple>					m_ripples;
	int									m_frame = 0;
	bool								m_isJustUsed = false;
};
