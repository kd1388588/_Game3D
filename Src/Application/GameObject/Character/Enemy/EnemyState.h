#pragma once

class Enemy;

// 基底クラス
class EnemyState
{
public:
	EnemyState() {}
	virtual ~EnemyState() {}
	virtual void ChangeState(Enemy* enemy) = 0;
	virtual void Update(Enemy* enemy) = 0;

protected:

	// ターゲットへの水平方向のベクトルを取得（ターゲットがいなければ false）
	static bool GetFlatVecToTarget(Enemy* enemy, Math::Vector3& outVec);
};

// 派生クラス
class EnemyStateIdle : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;
};

class EnemyStateChase : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;
};

class EnemyStateAttack : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;

private:
	bool  m_isCritical = false;
};

// 飛びかかり攻撃（ボス）：構える → 飛び上がってプレイヤーへ飛ぶ → 着地で周りを攻撃 → すき
class EnemyStateJumpAttack : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;

private:
	enum class Phase
	{
		Windup,	// 構え（しゃがんで力をためる）
		Rise,	// 飛び上がって上がっている途中（Start）
		Hang,	// 一番高い所から落ちている途中（Loop）
		Smash,	// 着地・叩きつけ・すき（End）
	};

	// 叩きつけるアニメーションを始める（framesToLand：何フレーム後に着地するか）
	void StartSmashAnim(Enemy* enemy, int framesToLand);

	Phase	m_phase = Phase::Windup;
	int		m_timer = 0;			// その段階になってからのフレーム数
	bool	m_isSmashDone = false;	// 叩きつけの攻撃をしたか
};

class EnemyStateDamage : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;
};

class EnemyStateDead : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;
};