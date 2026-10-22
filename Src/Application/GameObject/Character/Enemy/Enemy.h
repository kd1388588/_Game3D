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
	void GenerateDepthMapFromLight() override;	// 影

	void OnDamage(int damage, bool isCritical = false) override;
	void TurnToPlayer();

	// プレイヤーの方へ少しずつ向きを変える（1フレームに maxRadPerFrame ラジアンまで）
	void TurnToPlayerSmooth(float maxRadPerFrame);
	static float GetAttackTurnSpeed();	// 攻撃の構えの間に向きを変える速さ

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

	// ---- 飛びかかり攻撃 ----
	// 使えるか（CT（クールタイム）が終わっていて、アニメーションが読み込めている）
	bool CanJumpAttack() const;

	// 使った後のCTを始める
	void StartJumpAttackCooldown();

	// 普通の攻撃の後のすき（この間は追いかけるが、殴ってこない）
	void SetAttackCooldown(int frame) { m_attackCT = frame; }
	bool CanAttack() const { return m_attackCT <= 0; }

	// target へ向かって飛び上がる（放物線で飛び、空中にいる時間は高さから決まる）
	void StartLeap(const Math::Vector3& target);

	// 上がりきるまでのフレーム数（飛び上がるアニメーションの再生速度を合わせるのに使う）
	float GetLeapRiseFrames() const;

	// 空中での移動（毎フレーム呼ぶ）
	void UpdateLeapMove();

	// 水平方向の移動を止める（着地した時）
	void StopLeap();

	// 着地したか（一度落ち始めた後、地面に着いたら true）
	bool IsLeapLanded() const;

	// 着地の衝撃：周りの円の中にいるプレイヤーにダメージを与える
	void GroundSmash();

	// デバッグ：すぐに倒す（倒れる演出・撃破エフェクトは普通に倒した時と同じ）
	void DebugKill();

	// 振り下ろした手が地面に着いた所に、爪のエフェクト（_Claw2）を出す
	void PlaySlamEffect();

	// 足元の近く（体の真下・すぐ前）にいるプレイヤーへの攻撃判定
	// （腕の判定は体の前の方を通るので、足元まで近づかれると当たらなかったため）
	void AttackHitNear(bool isCritical);

	// 攻撃中にのけぞらないようにする（ダメージは受ける）
	void SetSuperArmor(bool isSuperArmor) { m_isSuperArmor = isSuperArmor; }

	// のけぞりを続けさせる（プレイヤーのATBコマンドの技を受けている間など。frames：続ける長さ）
	void AddStun(int frames) { m_stunTimer = std::max(m_stunTimer, frames); }
	bool IsStunned() const { return m_stunTimer > 0; }

	float GetBodyScale() const { return m_scale; }	// 体の大きさ（エフェクトの大きさ・高さに使う）

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

	// 飛びかかり攻撃
	int								m_jumpAttackCT = 0;			// CTの残りフレーム
	int								m_attackCT = 0;				// 普通の攻撃をもう一度できるまでの残りフレーム
	int								m_jumpAttackCoolTime = 0;	// 使った後のCT（敵の種類ごと）
	Math::Vector3					m_leapVelocity = Math::Vector3::Zero;	// 空中での水平方向の移動量（1フレーム）
	bool							m_isLeapFalling = false;	// 飛び上がった後、落ち始めたか
	bool							m_isSuperArmor = false;		// のけぞらない
	int								m_stunTimer = 0;			// のけぞりを続ける残りフレーム
};