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

	void ChangeState(std::shared_ptr<EnemyState> newState);
	void AttackHit(const Math::Matrix& hitMatrix, const Math::Vector3& extents, bool isCritical);

	bool IsAnimEnd() const { return m_animator.IsAnimationEnd(); }
	
	std::weak_ptr<Player> GetTarget() const { return m_wpTarget; }
	float GetAnimTime() const { return m_animator.GetTime(); }
	int GetCritRate() const { return m_critRate; }

	void SetTarget(std::weak_ptr<Player> target) { m_wpTarget = target; }
	void SetRotY(float rotY) { m_rotY = rotY; }
	void SetDead() { m_isExpired = true; }
	void SetScale(float scale) { m_scale = scale; }
	void SetHp(int hp) { m_hp = hp; m_maxHp = hp; }

	Math::Matrix GetRightArmMatrix() const;

	void SetUseRootMotion(bool use) { m_useRootMotion = use; }
	void SetRootMotionScale(float scale) { m_rootScale = scale; }

	float GetSearchRange() const { return m_searchRange; }
	float GetAttackRange() const { return m_attackRange; }

private:

	void Release();

	std::shared_ptr<EnemyState>		m_state = nullptr;
	std::shared_ptr<KdModelData>	m_swordModel = nullptr;
	std::weak_ptr<Player>			m_wpTarget;

	Math::Matrix					m_swordWorld;
	bool							m_isBoss = false;
	bool							m_useRootMotion = false;
	float							m_rootScale = 1.0f;
	float							m_rotY = 0.0f;
	float							m_scale = 3.0f;
	float							m_searchRange = 15.0f; // プレイヤーに気づく距離
	float							m_attackRange = 2.5f;  // 攻撃を開始する距離
	int								m_hp = 50;
	int								m_maxHp = 50;
	int								m_invincibleTimer = 0;
	int								m_critRate = 5;
};