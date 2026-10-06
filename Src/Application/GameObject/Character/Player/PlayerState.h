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

	// 前方へ減速しながら移動する（回避・ダッシュ用）
	static void MoveForwardWithDecay(Player* player, float baseSpeed, float moveFrame, bool isFlat);

	// 待機・移動中に共通の入力によるステート遷移（遷移した場合は true）
	static bool TryCommonTransition(Player* player);
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

	int m_comboType = 1;
	int m_comboStep = 1;
	int m_subStep = 0;
	bool m_isCritical = false;
	bool m_nextAttackReserved = false;

	// エフェクト再生フラグ
	bool m_isEffect = false;
	bool m_isHit = false;	// ※現在は常にfalse（多段ヒットさせるため）

	int m_loopTimer = 0;

	// キャンセル可能になるフレームを取得する関数
	float GetCancelFrame() const;

	// 現在のステートから再生するべきアニメーション名を取得する関数
	std::string GetAnimName() const;

	// 4-3（Start/Loop/Endに分かれた攻撃）かどうか
	bool IsSplitAttack() const { return m_comboType == 4 && m_comboStep == 3; }

	// ルートモーションによる前進を止める攻撃かどうか
	bool IsRootMotionLocked() const;

	// 次の段の攻撃へ移行（5段目以降はIdleへ）
	void ChangeToNextAttack(Player* player) const;
};

// 回避
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

	int  m_jumpPhase = JumpPhaseNone;
	int  m_jumpCount = 0;
	bool m_isFalling = false;
	bool m_isLandingAnimStarted = false;	// 着地アニメーションを開始済みか
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