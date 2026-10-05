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
	bool  m_hasAttacked = false; 
};

class EnemyStateDamage : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;

private:
	int m_timer = 0;
};

class EnemyStateDead : public EnemyState
{
public:
	void ChangeState(Enemy* enemy) override;
	void Update(Enemy* enemy) override;
};