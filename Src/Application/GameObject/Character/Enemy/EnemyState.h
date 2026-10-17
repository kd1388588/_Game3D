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
		Start,	// 構え（飛び上がる前）
		Air,	// 空中
		End,	// 着地・叩きつけ・すき
	};

	Phase	m_phase = Phase::Start;
	int		m_airTimer = 0;		// 飛び上がってからのフレーム数
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