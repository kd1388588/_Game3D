#pragma once
#include "../Base/BaseChara.h"
#include "../../../Utility/InputHelper.h"

// バトルモード（FF7リメイクのクラウドのように、Eキーで切り替える）
enum class BattleMode
{
	Operator,	// オペレーターモード：素早く動ける。時間でもATBゲージがたまる（左クリックは攻撃01）
	Punisher	// パニッシャーモード：動きは遅いが攻撃が重い。攻撃を当てるとATBゲージが多くたまる（左クリックは攻撃02）
};

// アビリティ（ATBコマンドメニューで選ぶ技。ATBゲージを使い、使った後はCT（クールタイム）がある）
enum class AbilitySlot
{
	Slot1 = 0,	// 攻撃04（ATBゲージ1本）
	Slot2 = 1,	// 攻撃05（ATBゲージ2本。使った後しばらくオペレーターに固定）
	Num
};

class TPSCamera;
class PlayerState;
class BaseScene;
class HPGage;
class ATBGage;
class ATBCommandMenu;
class LockOnMarker;
class Enemy;

class Player : public BaseChara
{
public:

	Player() {}
	~Player()			override;

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
	// ボタンの入力（先行入力つき）
	// 押してから kInputBufferFrame フレームの間は「押された」と答える（1回答えたら消える）
	// （技の終わり際など、少し早めに押しても受け付けて、操作の反応を良くする）
	bool CheckAttackInput()		{ return ConsumeInput(m_attackBuffer); }	// 左クリック（攻撃）
	bool CheckJumpInput()		{ return ConsumeInput(m_jumpBuffer); }		// Space（ジャンプ）
	bool CheckEvadeInput()		{ return ConsumeInput(m_evadeBuffer); }		// 右クリック（回避）

	// ATBコマンドメニューを開いているか（開いている間は時間がゆっくりになる）
	bool IsCommandMenuOpen() const { return m_isCommandOpen; }

	// ロックオン中の敵（いなければ nullptr）
	std::shared_ptr<Enemy> GetLockTarget() const { return m_wpLockTarget.lock(); }

	// ロックオン中の敵の方へすぐ向く（攻撃・技を出す瞬間に呼ぶ。出した後は向きを変えない）
	void FaceLockTarget();

	// Shift を押している間（移動中はダッシュ、止まっている時はガード）
	bool IsShiftDown() const;
	// 入力方向へ向きを変え、moveByCode が true なら決まった速さで移動する
	// （false の時は向きを変えるだけ。移動はルートモーションに任せる）
	// 入力が無ければ false を返す
	bool MoveProcess(bool moveByCode = true);

	// dir の方向へ向きを変える（1フレームに maxAngle 度まで）
	void TurnToward(const Math::Vector3& dir, float maxAngle);

	// カメラの向きを基準にした入力方向（長さ1）を取得する（入力が無ければ false）
	bool GetMoveInputDir(Math::Vector3& outDir) const;

	// 前のフレームに水平方向へ動いた量（ジャンプで勢いを引き継ぐために使う）
	const Math::Vector3& GetLastMoveDelta() const { return m_lastMoveDelta; }

	// ステート（アニメーション）関連
	float GetGravity() const { return m_gravity; }
	bool IsOnGround() const;	// 接地中（ジャンプ可能）かどうか

	// バトルモード（オペレーター / パニッシャー）
	BattleMode GetBattleMode() const { return m_battleMode; }
	bool IsPunisherMode() const { return m_battleMode == BattleMode::Punisher; }
	void SetBattleMode(BattleMode mode);			// モードを変える（攻撃パターンもそのモードのものに変わる）
	float GetModeMoveRate() const;					// 今のモードの移動の速さの倍率
	float GetModeAttackSpeedRate() const;			// 今のモードの攻撃の速さの倍率

	// ATBゲージ（1本 = kAtbBarMax。kAtbBarNum 本までためられる）
	static constexpr int	kAtbBarNum	= 2;
	static constexpr float	kAtbBarMax	= 100.0f;
	float GetAtbGage() const { return m_atbGage; }
	int GetAtbBarCount() const { return static_cast<int>(m_atbGage / kAtbBarMax); }	// たまっている本数
	void AddAtbGage(float val);

	// アビリティを使う（ATBゲージとCTを確かめ、使えたらゲージを減らしてCTを始める）
	// 使えたら、出す攻撃の種類（04・05）を返す。使えなければ 0
	int TryStartAbility(AbilitySlot slot);

	// アビリティの攻撃中か（アビリティの攻撃ではATBゲージはたまらない）
	void SetAbilityAttack(bool isAbility) { m_isAbilityAttack = isAbility; }

	// ガード中か（ガードのステートが毎フレーム true にする。受けるダメージが減り、のけぞらない）
	void SetGuarding(bool isGuarding) { m_isGuarding = isGuarding; }

	// ガードできるか（崩された後しばらくはできない）
	bool CanGuard() const { return m_guardBreakTimer <= 0; }

	// ガードしながら歩く速さ（1フレームの移動量）
	float GetGuardMoveSpeed() const;

	// ガードしながら moveDir の方向へ歩く時のアニメーション名と、その再生速度を選ぶ
	const std::string& SelectGuardWalkAnim(const Math::Vector3& moveDir, float moveSpeed, float& outAnimSpeed) const;

	// カメラの向いている方へ向きを変える（ガード中は敵の方を向いたまま歩けるように）
	void TurnTowardCameraForward(float maxAngle);

	// 着地アニメーションで両足が接地しているフレーム
	float GetLandingFrame(const std::string& animName) const;

	// ジャンプ開始アニメーションで、足が地面から離れる（踏み切る）フレーム（無ければ 0）
	float GetTakeoffFrame(const std::string& animName) const;

	// 空中で腰が上がってよい高さ（空中ループのアニメーションの腰の高さ。これより上はプログラムのジャンプに任せる）
	float GetAirPelvisMaxY() const { return m_airPelvisMaxY; }

	// 溜め攻撃（4-3）の End で、剣が地面に叩きつけられる（一番低くなる）フレーム（無ければ -1）
	float GetSwordSlamFrame() const { return m_swordSlamFrame; }

	// 左クリックで出す攻撃の種類（オペレーター：01、パニッシャー：02）
	int GetCurrentAttackType() const { return m_currentAttackType; }

	const Math::Matrix& GetSwordMatrixR() const { return m_swordWorldR; }
	const Math::Matrix& GetSwordMatrixL() const { return m_swordWorldL; }

	// ステータス関連
	void SetInvincibleTimer(int timer) { m_invincibleTimer = timer; }	// 無敵時間を設定
	void ExecJump(float power = 0.35f) { m_gravity = -power; }			// ジャンプを実行
	void Expire() { m_isExpired = true; }								// プレイヤーを消滅させる（ゲームオーバー用）

	// デバッグ用
	void Revive() { m_hp = m_maxHp; }

	// HPが0になったかどうか
	bool IsDead() const { return m_hp <= 0; }

	// ボックス（OBB）による攻撃判定
	// 敵に当たったら、その位置に hitEffectName のエフェクトを出す（空ならいつものヒットエフェクト）
	// damageRate：ダメージの倍率（アビリティなど）
	bool AttackOBB(const Math::Matrix& swordMatrix, bool isCritical, const std::string& hitEffectName = "", float damageRate = 1.0f);

	// 攻撃が当たった位置に、プレイヤーの攻撃の向きでヒットエフェクトを出す
	void PlayHitEffect(const std::string& effectName, const Math::Vector3& hitPos, bool isCritical) const;


	// 被弾判定
	int GetCritRate() const { return m_critRate; }
	void OnDamage(int damage, bool isCritical = false) override;


	// 現在の剣の位置を左右の軌跡に追加する（攻撃中に毎フレーム呼ぶ）
	void AddSwordTrailPoints();

	Math::Vector3 GetCameraTargetPos() const;

	// 現在の向き（Y軸回転）をラジアンで取得
	float GetAngleRad() const { return DirectX::XMConvertToRadians(m_angle); }

	// デバッグ用（KdDebugGUIから編集される）
	// 武器所持位置
	static	Math::Vector3			s_weaponRotR;
	static	Math::Vector3			s_weaponPosR;
	static	Math::Vector3			s_weaponRotL;
	static	Math::Vector3			s_weaponPosL;

	static int						s_currentAttackType;		// 現在の攻撃パターン

	// 見た目の確認用の切り替え（Player Parameter Editor の「Animation Move」タブから変更）
	static bool						s_useRootMotion;			// true：アニメーションの動きどおりに移動する
	static bool						s_bladeShapeHitbox;			// true：攻撃判定・軌跡を剣のメッシュに合わせる
	static float					s_throwAngleOffset;			// 剣を投げる攻撃の向きの微調整（度）
	static float					s_runSpeedRate;				// 走る速さの倍率（1.0 でアニメーションと同じ速さ）

private:

	void Release();

	void UpdateWorldMatrix();
	void UpdateWeaponMatrix();

	// アニメーションで腰が動いた分だけ、プレイヤーを移動させる（ルートモーションを使う時だけ）
	void ApplyAnimationMotion();

	// カメラの注視点の高さを更新する
	// （腰の上下の動きには付いていかず、足元の高さ（ジャンプ・階段など）になめらかに付いていく）
	void UpdateCameraTargetY();

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

	// 指定したアニメーションで、剣先が一番低くなる（地面に叩きつける）フレームを求める（無ければ -1）
	float CalcSwordLowestFrame(const std::string& animName) const;



	// ボタンの先行入力を更新する（毎フレーム1回）
	void UpdateInputBuffer();

	// 先行入力をすべて消す（メニューを開く・閉じる時に、メニューの操作が攻撃などに使われないように）
	void ClearInputBuffers();

	// ロックオン（ホイールクリック・R キーで、近くの敵に視点を合わせる／外す）
	void UpdateLockOn();

	// ロックオンできる一番近い敵を探す（exclude の敵は除く。いなければ nullptr）
	std::shared_ptr<Enemy> FindLockTarget(const Enemy* exclude = nullptr) const;

	// ロックオンのマークの位置を更新する
	void UpdateLockOnMarker();

	// バトルモードを切り替えた時に、体のまわりを光らせる（簡単なオーラ）

	// ATBコマンドメニュー（F キーで開き、技を選ぶと自動で最後まで出す）
	// 開いている間は true を返す
	bool UpdateCommandMenu();
	void OpenCommandMenu();
	void CloseCommandMenu();

	// 技を選んだ（使えたら技を出してメニューを閉じる）
	void DecideCommand(int index);

	// メニューで技を選べるか（ATBゲージが足りて、CT中でない）
	bool CanUseAbility(int index) const;

	// 先行入力が残っていれば消して true を返す
	static bool ConsumeInput(int& bufferTimer);
	void UpdateHpGage();

	// Eキーでバトルモードを切り替える
	void UpdateModeSwitch();

	// ATBゲージを時間でためて、UIに伝える
	void UpdateAtb();

	// アビリティのCT（クールタイム）・モードの固定時間・ガードの耐久値を進める
	void UpdateAbilityTimers();

	// アニメーションで1フレームに進む距離（pOutLocalDir を渡すと、キャラの向きから見た進む向きも受け取れる）
	float CalcAnimTravelSpeed(const std::string& animName, Math::Vector3* pOutLocalDir = nullptr) const;

	// ガード歩きのアニメーションの向きと速さを調べる（最初の1回）
	void SetupGuardWalkAnims();

	struct GuardWalkAnim
	{
		std::string		name;
		Math::Vector3	localDir = { 0.0f, 0.0f, 1.0f };	// キャラの向きから見た進む向き（長さ1）
		float			travelSpeed = 0.0f;					// 1フレームに進む距離（0 ならアニメーションに進む動きが無い）
	};
	std::vector<GuardWalkAnim>		m_guardWalkAnims;
	float							m_runTravelSpeed = 0.0f;	// 走りのアニメーションで1フレームに進む距離

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

	// 剣の行列を計算
	// （nodeNames を上から順に探し、最初に見つかったノードに付ける）
	Math::Matrix CalcSwordMatrix(const std::vector<std::string>& nodeNames, const Math::Vector3& rot, const Math::Vector3& pos) const;

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
	static std::shared_ptr<KdTexture> CreateSwordTrailTexture(const Math::Vector3& trailColor);

	// 軌跡の画像を読み込む（無い時は fallbackColor の色でプログラムから作る）
	static std::shared_ptr<KdTexture> LoadSwordTrailTexture(const std::string& path, const Math::Vector3& fallbackColor);

	// 軌跡の画像を、今のモードのものに切り替える
	void ApplySwordTrailTexture();


	std::shared_ptr<KdModelData>	m_swordModel = nullptr;
	std::shared_ptr<PlayerState>	m_state = nullptr;
	SwordTrail						m_swordTrailR;						// 右手の剣の軌跡
	SwordTrail						m_swordTrailL;						// 左手の剣の軌跡
	std::shared_ptr<KdTexture>		m_operatorTrailTex = nullptr;		// 軌跡の画像（オペレーター：青）
	std::shared_ptr<KdTexture>		m_punisherTrailTex = nullptr;		// 軌跡の画像（パニッシャー：オレンジ）

	bool							m_isTrailActive = false;			// 軌跡を描いている途中か
	bool							m_isTrailAddedThisFrame = false;	// このフレームに軌跡を追加したか

	std::unordered_map<std::string, float>		 m_landingFrames;	// 着地アニメ名 → 両足が接地するフレーム
	std::unordered_map<std::string, float>		 m_takeoffFrames;	// ジャンプ開始アニメ名 → 足が地面から離れるフレーム
	float										 m_swordSlamFrame = -1.0f;	// 4-3 の End で剣を地面に叩きつけるフレーム
	float										 m_airPelvisMaxY = -1.0f;	// 空中で腰が上がってよい高さ

	std::weak_ptr<HPGage>			m_wpHpGage;
	std::weak_ptr<ATBGage>			m_wpAtbGage;
	std::weak_ptr<ATBCommandMenu>	m_wpCommandMenu;
	std::weak_ptr<LockOnMarker>		m_wpLockOnMarker;
	std::weak_ptr<Enemy>			m_wpLockTarget;			// ロックオン中の敵
	bool							m_isLimLight = false;	// リムライトを光らせている途中か
	float							m_limPow = 0.0f;		// リムライトの pow（大きいほどふちだけが光る）
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

	// ステータス系
	float							m_angle = 0;				// Y軸回転角度（度）
	int								m_critRate = 10;			// クリティカル率（％）
	int								m_currentAttackType = 1;	// 攻撃アニメーションの番号

	// バトルモード・ATB
	BattleMode						m_battleMode = BattleMode::Operator;
	int								m_modeLockTimer = 0;		// モードを切り替えられない残りフレーム（攻撃05の後）
	int								m_abilityCT[static_cast<int>(AbilitySlot::Num)] = {};	// アビリティごとのCTの残りフレーム
	bool							m_isGuarding = false;		// ガード中か
	float							m_guardDurability = 100.0f;	// ガードの耐久値（0 になると崩される）
	int								m_guardRegenDelay = 0;		// 耐久値が回復し始めるまでの残りフレーム
	int								m_guardBreakTimer = 0;		// 崩された後、ガードできない残りフレーム
	bool							m_isCommandOpen = false;	// ATBコマンドメニューを開いているか
	int								m_commandCursor = 0;		// メニューで選んでいる技
	float							m_atbGage = 0.0f;			// ATBゲージ（0 ～ kAtbBarMax × kAtbBarNum）
	bool							m_isAbilityAttack = false;	// アビリティの攻撃中か

	// フラグ関係

	// 入力（押した瞬間の判定用）
	InputHelper::KeyTrigger			m_attackKey{ VK_LBUTTON };
	InputHelper::KeyTrigger			m_evadeKey{ VK_RBUTTON };
	InputHelper::KeyTrigger			m_abilityKeys[static_cast<int>(AbilitySlot::Num)] = { InputHelper::KeyTrigger{ '1' }, InputHelper::KeyTrigger{ '2' } };	// メニューで直接選ぶ

	// ATBコマンドメニューの操作
	InputHelper::KeyTrigger			m_lockOnKey{ VK_MBUTTON };	// ロックオン（ホイールクリック）
	InputHelper::KeyTrigger			m_lockOnSubKey{ 'R' };		// ロックオン（R キー）
	InputHelper::KeyTrigger			m_commandKey{ 'F' };		// 開く・閉じる
	InputHelper::KeyTrigger			m_menuUpKey{ 'W' };
	InputHelper::KeyTrigger			m_menuDownKey{ 'S' };
	InputHelper::KeyTrigger			m_menuUpArrowKey{ VK_UP };
	InputHelper::KeyTrigger			m_menuDownArrowKey{ VK_DOWN };
	InputHelper::KeyTrigger			m_menuDecideKey{ VK_RETURN };
	InputHelper::KeyTrigger			m_menuBackKey{ VK_BACK };
	InputHelper::KeyTrigger			m_modeKey{ 'E' };
	InputHelper::KeyTrigger			m_jumpKey{ VK_SPACE };

	// 先行入力の残りフレーム（0 なら押されていない）
	int								m_attackBuffer = 0;
	int								m_jumpBuffer = 0;
	int								m_abilityBuffer[static_cast<int>(AbilitySlot::Num)] = {};
	int								m_evadeBuffer = 0;

	BaseScene*						m_owner = nullptr;

	// カメラの注視点
	float							m_cameraHeight = 0.0f;		// 足元からの高さ
	float							m_cameraTargetY = 0.0f;		// 今の注視点の高さ（なめらかに変化させる）

	// 前のフレームからの移動量（水平方向）
	Math::Vector3					m_prevFramePos = Math::Vector3::Zero;
	Math::Vector3					m_lastMoveDelta = Math::Vector3::Zero;
};
