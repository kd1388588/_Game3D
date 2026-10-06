#pragma once
#include "../Base/BaseChara.h"
#include "../../../Utility/InputHelper.h"

enum class WeaponState
{
	Sheathed,	// 収めている状態
	Equipped	// 装備している状態
};

class TPSCamera;
class PlayerState;
class BaseScene;
class HPGage;

class Player : public BaseChara
{
public:

	Player() {}
	~Player()			override {}

	void Init()			override;

	void Update()		override;
	void PostUpdate()	override;

	void GenerateDepthMapFromLight() override;
	void DrawUnLit()	override;
	void DrawLit()		override;

	void SetOwner(BaseScene* owner) { m_owner = owner; }
	void SetCamera(const std::shared_ptr<TPSCamera>& camera) { m_camera = camera; }

	void ChangeState(const std::shared_ptr<PlayerState>& newState);

	// 入力関連
	bool CheckMoveInput() const;
	bool CheckEquipInput()	{ return m_equipKey.Update(); }
	bool CheckAttackInput()	{ return m_attackKey.Update(); }
	bool CheckJumpInput()	{ return m_jumpKey.Update(); }
	bool MoveProcess();

	// ステート（アニメーション）関連
	WeaponState GetWeaponState() const { return m_weaponState; }
	void SetWeaponState(WeaponState state) { m_weaponState = state; }
	bool IsCombatMode() const { return m_isCombatMode; }
	void SetCombatMode(bool isCombat) { m_isCombatMode = isCombat; }
	float GetEquipFrame() const { return m_equipFrame; }
	float GetUnequipFrame() const { return m_unequipFrame; }
	float GetGravity() const { return m_gravity; }
	bool IsOnGround() const;	// 接地中（ジャンプ可能）かどうか

	// 武器の装備状態に応じたアニメーション名を取得
	// 装備中は "Combat_" 付きのものを返す（用意されていなければ通常版）
	std::string GetWeaponAnimName(const std::string& baseName) const;

	// 着地アニメーションで両足が接地しているフレーム
	float GetLandingFrame(const std::string& animName) const;

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
	void AddAwakeningGage(float val);									// 覚醒ゲージを加算
	bool IsAwakening() const { return m_isAwakening; }					// 覚醒中かどうか
	void Expire() { m_isExpired = true; }								// プレイヤーを消滅させる（ゲームオーバー用）

	// デバッグ用
	void Revive() { m_hp = m_maxHp; }

	// ボックス（OBB）による攻撃判定
	bool AttackOBB(const Math::Matrix& swordMatrix, bool isCritical);

	// ヒットストップ用タイマー
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

	// デバッグ用（KdDebugGUIから編集される）
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
	static float					s_attackRootScale;

private:

	void Release();

	void UpdateWorldMatrix();
	void UpdateWeaponMatrix();
	void UpdateAwakening();
	void UpdateAwakeningInput();
	void UpdateHpGage();
	void UpdateAttackTypeSwitch();

	// 入力方向（ローカル座標 X:左右 Z:前後）を取得
	Math::Vector3 GetInputDir() const;

	// 現在の向き（m_angle）をラジアンで取得
	float GetAngleRad() const { return DirectX::XMConvertToRadians(m_angle); }

	WeaponState						m_weaponState = WeaponState::Sheathed;

	std::shared_ptr<KdModelData>	m_swordModel = nullptr;
	std::shared_ptr<KdModelData>	m_scabbardModel = nullptr;
	std::shared_ptr<PlayerState>	m_state = nullptr;
	std::shared_ptr<KdTrailPolygon> m_swordTrail = nullptr;

	std::unordered_map<std::string, std::string> m_lazyAnimPaths;
	std::unordered_map<std::string, float>		 m_landingFrames;	// 着地アニメ名 → 両足が接地するフレーム

	std::weak_ptr<HPGage>			m_wpHpGage;
	std::weak_ptr<TPSCamera>		m_camera;
	Math::Matrix					m_swordWorldR;
	Math::Matrix					m_swordWorldL;
	Math::Matrix					m_scabbardWorldR;
	Math::Matrix					m_scabbardWorldL;

	// ステータス系
	float							m_angle = 0;				// Y軸回転角度（度）
	int								m_critRate = 10;			// クリティカル率（％）
	float							m_awakeningGage = 0.0f;		// 覚醒ゲージ
	float							m_maxAwakeningGage = 100.0f;// 最大覚醒ゲージ
	int								m_awakeningTimer = 0;		// 覚醒時間（フレーム）
	int								m_currentAttackType = 1;	// 攻撃アニメーションの番号
	int								m_hitStopTimer = 0;			// ヒットストップ用タイマー

	// フラグ関係
	bool							m_isCombatMode = false;		// 戦闘モードかどうか
	bool							m_isAwakening = false;		// 覚醒中かどうか
	float							m_equipFrame = 30.0f;		// 装備アニメーションのフレーム数
	float							m_unequipFrame = 25.0f;		// 収めるアニメーションのフレーム数

	// 入力（押した瞬間の判定用）
	InputHelper::KeyTrigger			m_attackKey{ VK_LBUTTON };
	InputHelper::KeyTrigger			m_equipKey{ 'E' };
	InputHelper::KeyTrigger			m_jumpKey{ VK_SPACE };
	InputHelper::KeyTrigger			m_attackTypeKey{ 'C' };

	BaseScene*						m_owner = nullptr;
};
