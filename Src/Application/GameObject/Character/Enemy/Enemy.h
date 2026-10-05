#pragma once
#include "../Base/BaseChara.h"

class Player;
class EnemyState;

class Enemy : public BaseChara
{
public:
	Enemy() {}
	~Enemy() override {}

	void Init() override;
	void PostUpdate() override;
	void Update() override;
	void DrawLit() override;

	void OnDamage(int damage, bool isCritical = false) override;
	void TurnToPlayer();

	void SetBoss(bool isBoss) { m_isBoss = isBoss; }
	bool IsBoss() const { return m_isBoss; }

	void ChangeState(const std::shared_ptr<EnemyState>& newState);
	void AttackHit(const Math::Matrix& hitMatrix, const Math::Vector3& extents, bool isCritical);

	std::weak_ptr<Player> GetTarget() const { return m_wpTarget; }
	int GetCritRate() const { return m_critRate; }

	void SetTarget(std::weak_ptr<Player> target) { m_wpTarget = target; }
	void SetRotY(float rotY) { m_rotY = rotY; }
	void SetDead() { m_isExpired = true; }
	void SetScale(float scale) { m_scale = scale; }
	void SetHp(int hp) { m_hp = hp; m_maxHp = hp; }

	Math::Matrix GetRightArmMatrix() const;

	float GetSearchRange() const { return m_searchRange; }
	float GetAttackRange() const { return m_attackRange; }

private:

	void Release();

	void UpdateWorldMatrix();

	// 他の敵と重ならないように押し出す
	void PushAwayFromOtherEnemies();

	std::shared_ptr<EnemyState>		m_state = nullptr;
	std::weak_ptr<Player>			m_wpTarget;

	bool							m_isBoss = false;
	float							m_rotY = 0.0f;			// Y軸回転角度（ラジアン）
	float							m_scale = 3.0f;
	float							m_searchRange = 15.0f;	// プレイヤーに気づく距離
	float							m_attackRange = 2.5f;	// 攻撃を開始する距離
	int								m_critRate = 5;
};