#pragma once
#include "../Base/BaseChara.h"

enum class WeaponState
{
	Sheathed,	// 収めている状態
	Equipped	// 装備している状態
};

class TPSCamera;
class PlayerState;
class GameScene;
class HPGage;
class kdTrailPolygon;

class Player : public BaseChara
{

public:

	Player() {}
	~Player()			override {}

	void Init()			override;

	void Update()		override;
	void PostUpdate()	override;

	void GenerateDepthMapFromLight() override;
	void DrawUnLit()		override;
	void DrawLit()		override;

	void SetWeaponState(WeaponState state) { m_weaponState = state; }
	void SetCombatMode(bool isCombat) { m_isCombatMode = isCombat; }
	void SetCamera(std::shared_ptr<TPSCamera>_camera)
	{
		m_camera = _camera;
	}

	void ChangeState(std::shared_ptr<PlayerState> newState);

	bool CheckMoveInput();
	bool CheckEquipInput();
	bool CheckAttackInput();
	bool CheckJumpInput();
	bool MoveProcess();

	// ステート（アニメーション）関連
	float GetAnimTime() const { return m_animator.GetTime(); }
	WeaponState GetWeaponState() const { return m_weaponState; }
	float GetEquipFrame() const { return m_equipFrame; }
	float GetUnequipFrame() const { return m_unequipFrame; }
	float GetGravity() const { return m_gravity; }
	bool IsAnimEnd() const { return m_animator.IsAnimationEnd(); }
	bool IsCombatMode() const { return m_isCombatMode; }
	void SetUseRootMotion(bool use) { m_useRootMotion = use; }
	void SetRootMotionScale(float scale) { m_rootScale = scale; }

	// 攻撃アニメーション
	void ChangeAnimationLazy(const std::string& animName, bool isLoop = true, bool forceRestart = false, float blendFrame = 0.0f);
	void RegisterAnimPath(const std::string& name, const std::string& path) { m_lazyAnimPaths[name] = path; }
	int GetCurrentAttackType() const { return m_currentAttackType; }
	void SetCurrentAttackType(int type) { m_currentAttackType = type; }

	const Math::Matrix& GetSwordMatrixR() const { return m_swordWorldR; }
	const Math::Matrix& GetSwordMatrixL() const { return m_swordWorldL; }

	// ステータス関連
	void SetInvincibleTimer(int timer) { m_invincibleTimer = timer; }	// 無敵時間を設定
	void ExecJump(float power = 0.35f) { m_gravity = -power; }			// ジャンプを実行
	void AddAwakeningGage(float val)									// 覚醒ゲージを加算
	{
		m_awakeningGage += val;
		if (m_awakeningGage > m_maxAwakeningGage) m_awakeningGage = m_maxAwakeningGage;
	}
	bool IsAwakening() const { return m_isAwakening; }					// 覚醒中かどうか
	void Expire() { m_isExpired = true; }								// プレイヤーを消滅させる（ゲームオーバー用）


	// デバッグ用
	void Revive()
	{
		m_hp = 100;
	}

	// スウィープ判定
	void AttackHit(const Math::Vector3& prevPos, const Math::Vector3& currentPos, bool isCritical);
	
	// ボックス（カプセル）判定
	bool AttackOBB(const Math::Matrix& swordMatrix, bool isCritical);

	// タイマーを外からセットできるように追加
	void SetHitStopTimer(int timer) { m_hitStopTimer = timer; }

	// 被弾判定
	int GetCritRate() const { return m_critRate; }
	void OnDamage(int damage, bool isCritical = false) override;

	// 剣先座標取得
	Math::Vector3 GetSwordTipPositionR() const;
	Math::Vector3 GetSwordTipPositionL() const;
	// 剣元座標取得
	Math::Vector3 GetSwordBasePositionR() const;
	Math::Vector3 GetSwordBasePositionL() const;

	std::shared_ptr<KdTrailPolygon> GetSwordTrail() { return m_swordTrail; }

	Math::Vector3 GetCameraTargetPos() const;

	// デバッグ用
	// 武器所持位置
	static	Math::Vector3			s_weaponRotR;
	static	Math::Vector3			s_weaponPosR;
	static	Math::Vector3			s_weaponRotL;
	static	Math::Vector3			s_weaponPosL;

	// 納刀位置
	static	Math::Vector3			s_sheathedRotR;
	static	Math::Vector3			s_sheathedPosR;
	static	Math::Vector3			s_sheathedRotL;
	static	Math::Vector3			s_sheathedPosL;

	static int						s_currentAttackType;		// 現在の攻撃パターン
	static float					s_evadeSpeed;				// 回避の初速
	static float					s_dashSpeed;				// ダッシュの初速
	static float					s_attackRootScale;
	static float					s_manualStepSpeed;


	float							m_rootScale = 1.0f;			// 倍率変数

	void SetOwner(GameScene* _owner) { m_owner = _owner; }
protected:

	WeaponState m_weaponState = WeaponState::Sheathed;

private:

	void UpdateAnimation(bool isMove);
	void UpdateWeaponMatrix();
	void Release();

	std::shared_ptr<KdModelData>	m_swordModel = nullptr;
	std::shared_ptr<KdModelData>	m_scabbardModel = nullptr;
	std::shared_ptr<PlayerState>	m_state = nullptr;
	std::shared_ptr<KdTrailPolygon> m_swordTrail = nullptr;

	std::unordered_map<std::string, std::string> m_lazyAnimPaths;

	std::weak_ptr<HPGage>			m_wpHpGage;
	std::weak_ptr	<TPSCamera>		m_camera;
	Math::Matrix					m_swordWorldR;
	Math::Matrix					m_swordWorldL;
	Math::Matrix					m_scabbardWorldR;
	Math::Matrix					m_scabbardWorldL;

	// ステータス系
	int								m_hp = 100;					// 現在のHP
	int								m_maxHp = 100;				// 最大HP
	float							m_angle = 0;				// Y軸回転角度
	int								m_critRate = 10;			// クリティカル率（％）
	int								m_invincibleTimer = 0;		// 無敵時間（フレーム）
	float							m_awakeningGage = 0.0f;		// 覚醒ゲージ
	float							m_maxAwakeningGage = 100.0f;// 最大覚醒ゲージ
	int								m_awakeningTimer = 0;		// 覚醒時間（フレーム）
	int								m_currentAttackType = 1;	// 攻撃アニメーションの番号
	int								m_hitStopTimer = 0;			// ヒットストップ用タイマー
		
	// フラグ関係
	bool							m_prevClick = false;		// 前フレームのマウス左クリック状態
	bool							m_isCombatMode = false;		// 戦闘モードかどうか
	bool							m_isAwakening = false;		// 覚醒中かどうか
	bool							m_EKey = false;				// 前フレームのEキー入力状態
	bool							m_prevSpace = false;		// 前フレームのスペースキー入力状態
	bool							m_prevKeyC = false;			// 前フレームのCキー入力状態
	float							m_equipFrame = 30.0f;		// 装備アニメーションのフレーム数
	float							m_unequipFrame = 25.0f;		// 収めるアニメーションのフレーム数
	bool							m_useRootMotion = false;	// ルートモーションを使うかどうかのフラグ
	
	// デバッグ用
	float							m_inputLimitTime = 50.0f;	// コンボ入力受付時間
	float							m_changeTime = 25.0f;		// コンボアニメーション切り替え時間
	float							m_stepSpeed = 0.15f;		// ステップ移動速度
	
	GameScene* m_owner;
};