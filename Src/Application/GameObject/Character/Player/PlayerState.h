#pragma once
#include <memory>

class Player;

// 基底クラス
class PlayerState
{
public:
	PlayerState() {}
	~PlayerState() {}

	// 状態に入った瞬間に呼ばれる（アニメーション再生など）
	virtual void ChangeState(Player* player) = 0;

	// 毎フレーム呼ばれる（入力・移動処理など）
	virtual void Update(Player* player) = 0;
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
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateEquip : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateUnequip : public PlayerState
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateComboAttack : public PlayerState
{
public:
	// 引数なしコンストラクタ（念のため）
	PlayerStateComboAttack() {}

	// サブステート(subStep)はデフォルトで0。4-3の時だけ 1:Start, 2:Loop, 3:End として使う
	PlayerStateComboAttack
	(
		int comboType, 
		int comboStep, 
		int subStep = 0, 
		bool reserved = false
	)
		: 
		m_comboType(comboType), 
		m_comboStep(comboStep), 
		m_subStep(subStep), 
		m_nextAttackReserved(reserved) 
	{}

	void ChangeState(Player* player) override;
	void Update(Player* player) override;

private:

	std::shared_ptr<KdEffekseerObject>	m_effect = nullptr;
	std::shared_ptr<class Effect> m_trailEffect = nullptr;

	int m_comboType = 1;
	int m_comboStep = 1;
	int m_subStep = 0;
	bool m_isCritical = false;
	bool m_nextAttackReserved = false;

	// エフェクト再生フラグ
	bool m_isEffect = false;
	bool m_isHit = false;

	int m_loopTimer = 0;

	// キャンセル可能になるフレームを取得する関数
	float GetCancelFrame() const;

	// 現在のステートから再生するべきアニメーション名を取得する関数
	std::string GetAnimName() const;
};

// 防御
class PlayerStateEvade : public PlayerState 
{
public:
	void ChangeState(Player* player) override;
	void Update(Player* player) override;
};

class PlayerStateDash : public PlayerState
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
	int  m_jumpPhase = 0;
	int  m_jumpCount = 0;
	bool m_isFalling = false;
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