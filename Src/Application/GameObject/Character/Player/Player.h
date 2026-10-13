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
	void DrawEffect()	override;
	void DrawLit()		override;

	void SetOwner(BaseScene* owner) { m_owner = owner; }
	void SetCamera(const std::shared_ptr<TPSCamera>& camera) { m_camera = camera; }

	void ChangeState(const std::shared_ptr<PlayerState>& newState);

	// 入力関連
	bool CheckMoveInput() const;
	bool CheckEquipInput()	{ return m_equipKey.Update(); }
	bool CheckAttackInput()	{ return m_attackKey.Update(); }
	bool CheckJumpInput()	{ return m_jumpKey.Update(); }
	// 入力方向へ向きを変え、moveByCode が true なら決まった速さで移動する
	// （false の時は向きを変えるだけ。移動はルートモーションに任せる）
	// 入力が無ければ false を返す
	bool MoveProcess(bool moveByCode = true);

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

	// HPが0になったかどうか
	bool IsDead() const { return m_hp <= 0; }

	// ボックス（OBB）による攻撃判定
	// 敵に当たったら、その位置に hitEffectName のエフェクトを出す（空ならいつものヒットエフェクト）
	bool AttackOBB(const Math::Matrix& swordMatrix, bool isCritical, const std::string& hitEffectName = "");

	// 攻撃が当たった位置に、プレイヤーの攻撃の向きでヒットエフェクトを出す
	void PlayHitEffect(const std::string& effectName, const Math::Vector3& hitPos, bool isCritical) const;

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

	// 現在の剣の位置を左右の軌跡に追加する（攻撃中に毎フレーム呼ぶ）
	void AddSwordTrailPoints();

	Math::Vector3 GetCameraTargetPos() const;

	// 現在の向き（Y軸回転）をラジアンで取得
	float GetAngleRad() const { return DirectX::XMConvertToRadians(m_angle); }

	// プレイヤーの向きに合わせたエフェクト用の行列を作成
	// forwardOffset：前方へずらす距離、height：足元からの高さ、scale：大きさ
	Math::Matrix CreateEffectMatrix(float forwardOffset, float height, float scale) const;

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

	// 見た目の確認用の切り替え（Player Parameter Editor の「Animation Move」タブから変更）
	static bool						s_useRootMotion;			// true：アニメーションの動きどおりに移動する
	static bool						s_bladeShapeHitbox;			// true：攻撃判定・軌跡を剣のメッシュに合わせる
	static float					s_throwAngleOffset;			// 剣を投げる攻撃の向きの微調整（度）

private:

	void Release();

	void UpdateWorldMatrix();
	void UpdateWeaponMatrix();

	// アニメーションで腰が動いた分だけ、プレイヤーを移動させる（ルートモーションを使う時だけ）
	void ApplyAnimationMotion();

	// 剣を投げる攻撃で、投げた剣がまっすぐ前へ飛ぶように向きを補正する
	// （アニメーションのデータでは、剣が斜め前へ飛んでいるため）
	void ApplyThrowCorrection();

	// 剣1本分の補正（手から離れるほど強く、腰を中心に剣を回して前へ向ける）
	void CorrectThrownWeapon(int weaponIndex, int handIndex, float angleDeg);

	// 剣を投げるアニメーションで、剣が一番遠くまで飛んだ時の向きを調べ、
	// それをまっすぐ前に向けるための回転角度（度）を求める
	float CalcThrowCorrectionAngle(const std::string& animName, int weaponIndex) const;

	// モデルのノード番号を名前で探す（見つからなければ -1）
	int FindNodeIndex(const std::string& name) const;
	void UpdateAwakening();

	// 覚醒中のオーラエフェクトを更新（覚醒中は再生し続け、終わったら止める）
	void UpdateAwakeningEffect();
	void UpdateAwakeningInput();
	void UpdateHpGage();
	void UpdateAttackTypeSwitch();

	// 剣の軌跡（1本分）
	struct SwordTrail
	{
		std::shared_ptr<KdTrailPolygon>	polygon = nullptr;
		std::deque<Math::Vector3>		baseHistory;	// 刃の根元の位置の履歴（新しい順）
		std::deque<Math::Vector3>		tipHistory;		// 刃の先端の位置の履歴（新しい順）
	};

	// 剣の軌跡の更新（攻撃していないフレームは軌跡を徐々に消す）
	void UpdateSwordTrails();

	// 軌跡にポイントを追加する（前フレームとの間を曲線で補間して、なめらかな弧にする）
	void AddSwordTrailSample(SwordTrail& trail, const Math::Matrix& swordMat);

	// 軌跡を消す
	void ClearSwordTrail(SwordTrail& trail);

	// 軌跡を古い方から1フレーム分短くする
	void ShrinkSwordTrail(SwordTrail& trail);

	// 入力方向（ローカル座標 X:左右 Z:前後）を取得
	Math::Vector3 GetInputDir() const;

	// 鞘・剣の行列を計算
	// （nodeNames を上から順に探し、最初に見つかったノードに付ける）
	Math::Matrix CalcScabbardMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos) const;
	Math::Matrix CalcSwordMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos,
		const Math::Matrix& scabbardMat) const;

	// 候補の名前を上から順に探し、最初に見つかったノードを返す（無ければ nullptr）
	const KdModelWork::Node* FindFirstNode(const std::vector<std::string>& nodeNames) const;

	// モデルに含まれる剣のメッシュから求めた、刃の向きと長さ（メッシュのノードから見た座標）
	struct BladeShape
	{
		int				nodeIndex = -1;		// 剣のメッシュのノード番号（-1ならモデルに剣が無い）
		Math::Vector3	localHand;			// 刃の中心線の上で、手（ノードの原点）に一番近い点
		Math::Vector3	localDir;			// 手から刃先への向き（長さ1）
		float			tipLength = 0.0f;	// 手から刃先までの長さ
	};

	// 剣のメッシュの頂点から、刃の中心線（一番長く伸びている向き）と刃先の位置を求める
	// （候補の名前のメッシュが見つからなければ nodeIndex = -1 のまま返す）
	static BladeShape CalcBladeShape(const KdModelData& modelData, const std::vector<std::string>& meshNodeNames);

	// 刃の形から剣の行列を作る（原点：手、Y軸：刃先の方向。Y座標がそのまま手からの距離になる）
	Math::Matrix CalcBladeMatrix(const BladeShape& shape) const;

	// 読み込むアニメーションのリストを作成
	static std::vector<AnimLoadInfo> CreateAnimList();

	// GUIで調整した回転(度)・座標から、ノードに追従する行列を作成
	static Math::Matrix CreateAttachMatrix(const Math::Vector3& rotDeg, const Math::Vector3& pos,
		const Math::Matrix& nodeMat, const Math::Matrix& ownerMat);

	// 剣のローカルY軸（刃の方向）上の点をワールド座標に変換
	static Math::Vector3 GetSwordPoint(const Math::Matrix& swordMat, float localY);

	// 剣の軌跡ポリゴンに渡す行列を作成（刃の根元・先端の位置から）
	static Math::Matrix CreateTrailMatrix(const Math::Vector3& base, const Math::Vector3& tip);

	// 剣の軌跡用のテクスチャを作成する
	// （刃の先端ほど明るく、古い部分ほど暗くなるグラデーション。加算合成用）
	static std::shared_ptr<KdTexture> CreateSwordTrailTexture();


	WeaponState						m_weaponState = WeaponState::Sheathed;

	std::shared_ptr<KdModelData>	m_swordModel = nullptr;
	std::shared_ptr<KdModelData>	m_scabbardModel = nullptr;
	std::shared_ptr<PlayerState>	m_state = nullptr;
	SwordTrail						m_swordTrailR;						// 右手の剣の軌跡
	SwordTrail						m_swordTrailL;						// 左手の剣の軌跡
	std::shared_ptr<KdTexture>		m_swordTrailTex = nullptr;			// 軌跡のテクスチャ

	std::shared_ptr<KdEffekseerObject>	m_awakeningEffect = nullptr;	// 覚醒中のオーラエフェクト
	bool							m_isTrailActive = false;			// 軌跡を描いている途中か
	bool							m_isTrailAddedThisFrame = false;	// このフレームに軌跡を追加したか

	std::unordered_map<std::string, std::string> m_lazyAnimPaths;
	std::unordered_map<std::string, float>		 m_landingFrames;	// 着地アニメ名 → 両足が接地するフレーム

	std::weak_ptr<HPGage>			m_wpHpGage;
	std::weak_ptr<TPSCamera>		m_camera;
	Math::Matrix					m_swordWorldR;
	Math::Matrix					m_swordWorldL;

	// 刃の範囲（剣の行列のY座標。攻撃判定の箱と軌跡に使う）
	BladeShape						m_bladeShapeR;
	BladeShape						m_bladeShapeL;

	// 剣を投げる攻撃の補正
	int								m_weaponNodeR = -1;			// weapon_r のノード番号
	int								m_weaponNodeL = -1;			// weapon_l のノード番号
	int								m_handNodeR = -1;			// hand_r のノード番号
	int								m_handNodeL = -1;			// hand_l のノード番号
	int								m_pelvisNode = -1;			// pelvis のノード番号
	float							m_throwAngleR = 0.0f;		// 右の剣をまっすぐ前へ向ける角度（度）
	float							m_throwAngleL = 0.0f;		// 左の剣をまっすぐ前へ向ける角度（度）
	float							m_swordBladeBaseY = 0.0f;	// 刃の根元
	float							m_swordBladeTipY = 0.0f;	// 刃の先端
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
