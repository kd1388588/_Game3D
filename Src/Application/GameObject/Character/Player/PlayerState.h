#pragma once
#include <memory>

class Player;

// 基底クラス
class PlayerState
{
public:
	PlayerState() {}
	virtual ~PlayerState() {}

	// 状態に入った瞬間に呼ばれる（アニメーション再生など）
	virtual void ChangeState(Player* player) = 0;

	// 毎フレーム呼ばれる（入力・移動処理など）
	virtual void Update(Player* player) = 0;

protected:

	// 前方へ減速しながら移動する（回避用）
	static void MoveForwardWithDecay(Player* player, float baseSpeed, float moveFrame, bool isFlat);

	// 回避を始める（攻撃の途中から回避する時も、ここを通す）
	static void StartEvade(Player* player);

	// 待機・移動中に共通の入力によるステート遷移（遷移した場合は true）
	static bool TryCommonTransition(Player* player);

	// ルートモーション（アニメーションの動きどおりに移動する）を使う設定にする
	// プレイヤーの動きはすべてこれを使う（倍率は 1.0 ＝ アニメーションと同じ距離）
	static void UseRootMotion(Player* player);

};

// 派生クラス
class PlayerStateIdle : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateRun : public PlayerState
{
public:
	// isDash：最初から全力疾走で走る（急ブレーキの途中で、また走り出した時）
	explicit PlayerStateRun(bool isDash = false) : m_startInDash(isDash) {}

	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:
	// ダッシュの流れ
	// Shift を押して走る → 走りながら納刀 → 剣を納めて全力で走る
	// （Shift を離しても、移動キーを離すまで全力で走り続ける。止まったら PlayerStateDashStop へ）
	enum class DashPhase
	{
		None,		// 普段の走り（剣を抜いている）
		Sheathe,	// 走りながら納刀
		Dash,		// 剣を納めて全力で走る
	};
	void ChangeDashPhase(Player* player, DashPhase phase);

	bool		m_startInDash = false;
	DashPhase	m_dashPhase = DashPhase::None;
};

// ダッシュから止まる：急ブレーキ（Brake）→ 立ったまま抜刀（Draw）→ 待機
class PlayerStateDashStop : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:
	enum class Phase
	{
		Brake,
		Draw,
	};
	void ChangePhase(Player* player, Phase phase);

	Phase m_phase = Phase::Brake;
};

// ガード（止まっている時に Shift を押している間。受けるダメージが減り、のけぞらない）
// 構える（Start）→ 構えたまま（Loop）→ 下ろす（End）。攻撃を受けた時は Hit を再生する
class PlayerStateGuard : public PlayerState
{
public:
	enum class Phase
	{
		Start,	// 構え始め
		Loop,	// 構えたまま
		Hit,	// 攻撃を受け止めた
		End,	// 構えを解く（この間はガードしていない）
	};

	PlayerStateGuard(Phase phase = Phase::Start) : m_phase(phase) {}

	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:

	// ガードの段階を変えて、その段階のアニメーションを再生する
	void ChangePhase(Player* player, Phase phase);

	// 構えたまま歩く（移動キーの方向へ、普段の半分くらいの速さで。向きはカメラの方を向いたまま）
	// 移動キーを押していなければ、構えたまま止まる
	void UpdateGuardMove(Player* player);

	Phase m_phase = Phase::Start;
};

// ガードを崩された（よろけて、しばらくガードできない）
class PlayerStateGuardBreak : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateComboAttack : public PlayerState
{
public:
	// 4-3（溜め攻撃）だけで使うサブステート
	enum SubStep
	{
		SubStepNone		= 0,
		SubStepStart	= 1,
		SubStepLoop		= 2,
		SubStepEnd		= 3,
	};

	// 引数なしコンストラクタ（念のため）
	PlayerStateComboAttack() {}

	// サブステート(subStep)はデフォルトで0。4-3の時だけ 1:Start, 2:Loop, 3:End として使う
	// isAbility：アビリティ（ATBコマンドメニュー）で出した攻撃か（自動で最後の段まで出し、ダメージが上がる）
	PlayerStateComboAttack
	(
		int comboType, 
		int comboStep, 
		int subStep = 0, 
		bool reserved = false,
		bool isAbility = false
	)
		: 
		m_comboType(comboType), 
		m_comboStep(comboStep), 
		m_subStep(subStep), 
		m_nextAttackReserved(reserved),
		m_isAbility(isAbility)
	{}

	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:


	int m_comboType = 1;
	int m_comboStep = 1;
	int m_subStep = 0;
	bool m_isCritical = false;
	bool m_nextAttackReserved = false;
	bool m_isAbility = false;			// アビリティで出した攻撃か


	int m_loopTimer = 0;

	// キャンセル可能になるフレームを取得する関数
	float GetCancelFrame() const;

	// 現在のステートから再生するべきアニメーション名を取得する関数
	std::string GetAnimName() const;

	// 4-3（Start/Loop/Endに分かれた攻撃）かどうか
	bool IsSplitAttack() const { return m_comboType == 4 && m_comboStep == 3; }

	// 次の段の攻撃へ移行（5段目以降はIdleへ）
	void ChangeToNextAttack(Player* player) const;

	// 決めの攻撃の専用エフェクトの名前（無い攻撃は空。普通の攻撃は、当たるたびに斬撃1本だけ出る）
	const std::string& GetFinisherEffectName() const;

	// 今のフレームに剣の軌跡を出すか
	// （普通の攻撃は攻撃判定のフレームの間。溜め攻撃は、振り始めから地面に叩きつけるまでずっと）
	bool IsTrailFrame(const Player* player, float animTime) const;

};

// 回避
class PlayerStateEvade : public PlayerState 
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateJump : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:
	enum JumpPhase
	{
		JumpPhaseNone		= 0,
		JumpPhaseAir		= 1,	// 空中
		JumpPhaseLanding	= 2,	// 着地硬直
	};

	// 着地予測を行い、間に合うなら着地アニメーションを前倒しで開始する
	void TryStartLandingAnim(Player* player);

	// 着地アニメーションを開始する
	// framesToLand：何フレーム後に着地するか（このフレームで両足が接地している状態になるよう再生位置を合わせる）
	void StartLandingAnim(Player* player, int framesToLand);

	// 空中での移動（踏み切った時の勢いを保ち、入力の方向へ少しずつ曲がる）
	void UpdateAirMove(Player* player);

	// 踏み切る（キャラを上に飛ばす）
	void Takeoff(Player* player);

	int  m_jumpPhase = JumpPhaseNone;
	int  m_landingTimer = 0;				// 着地してからのフレーム数
	bool m_isFalling = false;
	bool m_isLandingAnimStarted = false;	// 着地アニメーションを開始済みか
	bool m_isTakeoffDone = false;			// 実際に跳んだか（アニメーションの踏み切りまで待つ）
	float m_takeoffFrame = 0.0f;			// 跳ぶフレーム（ジャンプ開始アニメーションで足が地面から離れるフレーム）

	Math::Vector3 m_airVelocity = Math::Vector3::Zero;	// 空中での水平方向の速さ（1フレームの移動量）
	float m_airSpeed = 0.0f;							// 空中で入力した時の速さ
};

class PlayerStateDamage : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateDead : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};