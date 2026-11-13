#pragma once

// ============================================================
// ゲームの調整用の設定（オブジェクトエディタで調整する値）
// ・F8 のデバッグ表示を出すと「Object Editor」のウィンドウが出て、ゲームを動かしたまま調整できる
// ・タブ：Player（動き・剣）／Enemy（HP の倍率・ブレイク）／Stage（霧）／UI（HP が少ない時の画面・HP ゲージの色）
// ・「保存」で Asset/Data/StageSettings.json に書き出し、次にゲームを始めた時に読み込む
// ・今は「霧」の項目だけ（距離の霧・高さの霧・地面の霧）。項目はここに足していく
// ・「UI」タブには、HP が少ない時の画面（白黒・暗さ・周辺減光）の設定もある
// ・「Enemy」タブには、敵の HP の倍率と、ボスのブレイクの設定がある
//
// 【役割】見た目の数値を 1 か所に持ち、ゲームを動かしたまま imgui で調整し、JSON に保存・読み込みする
// 【構成】・Fog / GroundMist / LowHpScreen：設定のまとまり（struct。初期値もここに書く）
//         ・ApplyFog   ：霧の値をシェーダーへ渡す（GameScene::Event で毎フレーム）
//         ・DrawEditor ：Stage Editor のウィンドウ（KdDebugGUI がデバッグ表示中に呼ぶ）
//         ・Save / Load：Asset/Data/StageSettings.json
// 【なぜエディタを作ったのか】霧は少しの数値の違いで見た目が大きく変わる
//         ビルドし直さずに、見ながら調整して、良い値を保存できるようにするため
// 【なぜシングルトンか】ステージ・HP ゲージ・Player・KdDebugGUI など、あちこちから同じ設定を読むため
// 【参考】nlohmann/json（JSON の読み書き）https://github.com/nlohmann/json
//         Dear ImGui https://github.com/ocornut/imgui
// ============================================================
class StageSettings
{
public:

	// 距離の霧・高さの霧（シェーダーの霧。KdAmbientController に渡す）
	struct Fog
	{
		bool			distanceEnable	= true;
		Math::Vector3	distanceColor	= { 0.10f, 0.11f, 0.16f };
		float			distanceDensity	= 0.007f;	// 濃さ（1 / exp(距離 × 濃さ) だけ元の色が残る）

		bool			heightEnable	= true;
		Math::Vector3	heightColor		= { 0.12f, 0.13f, 0.18f };
		float			heightTop		= 1.0f;		// この高さより下に霧がかかる
		float			heightBottom	= -1.0f;	// この高さで一番濃い
		float			heightDistance	= 45.0f;	// カメラからこの距離で一番濃くなる
	};

	// 地面の霧（床の近くに漂う霧のかたまり。GroundMist が描く）
	struct GroundMist
	{
		bool			enable			= true;
		int				count			= 70;		// かたまりの数
		float			brightness		= 0.22f;	// 濃さ（加算で重ねるので、明るさ＝濃さになる）
		float			brightnessRandom = 0.08f;	// かたまりごとの濃さのばらつき
		Math::Vector3	color			= { 0.62f, 0.65f, 0.78f };	// 色（青白い）
		float			sizeMin			= 5.0f;		// かたまりの幅（m）
		float			sizeMax			= 10.0f;
		float			flatten			= 0.35f;	// 高さ ÷ 幅（小さいほど平べったく、床をはうように見える）
		float			bottomHeight	= -0.1f;	// かたまりの下の端の高さ（床の上からの m。マイナスで床にしずむ）
		float			heightRandom	= 0.25f;	// 高さのばらつき（m）
		float			range			= 36.0f;	// カメラのまわり、何 m の範囲に出すか（外に出たら反対側に出し直す）
		float			nearFadeStart	= 3.0f;		// カメラにこれより近いと薄くする（目の前が白くならないように）
		float			nearFadeLength	= 3.0f;		// 薄くし始めてから、完全に消えるまでの距離
		float			driftSpeed		= 0.6f;		// 流れる速さ（m/秒）
		float			driftAngle		= 30.0f;	// 流れる向き（度。0 で奥（+Z）へ、90 で右（+X）へ）
		float			swaySpeed		= 0.4f;		// 濃さのゆらぎの速さ
		float			swayRange		= 0.3f;		// 濃さのゆらぎの強さ（0：ゆらがない）
	};

	// HP が少ない時の画面（白黒・暗く・周辺減光）。HP が start の割合より少なくなると始まり、HP 0 で一番強くなる
	// 倒れた時は、そこからさらに真っ黒になるまで暗くして、ゲームオーバーへ移る
	struct LowHpScreen
	{
		bool	enable			= true;
		float	startRate		= 0.1f;		// この HP の割合（0～1）より少なくなると始まる
		float	grayscale		= 0.85f;	// HP 0 の時の白黒の強さ（1 で完全な白黒。少しだけ色を残すため 0.85）
		float	darken			= 0.35f;	// HP 0 の時に、全体を暗くする強さ
		float	vignette		= 0.97f;	// HP 0 の時の周辺減光の強さ（1 でふちが真っ黒）
		float	vignetteStart	= 0.25f;	// 周辺減光が始まる位置（0：真ん中、1：上下左右のふち）
		float	followRate		= 0.05f;	// HP が変わった時に、画面がその強さに近づく速さ（1フレームに近づく割合。小さいほどゆっくり）
		// HP ゲージの色：半分より下で黄色 → 4 分の 1 より下でゆっくり赤く点滅 → startRate より下（画面が暗くなり始める）で速く点滅
		float	yellowRate		= 0.5f;		// HP ゲージが黄色になる HP の割合
		float	blinkRate		= 0.25f;	// HP ゲージが赤く点滅し始める HP の割合
		float	blinkSpeed		= 0.08f;	// ゆっくりの点滅の速さ
		float	fastBlinkSpeed	= 0.2f;		// 速い点滅の速さ（startRate より下）
		float	deathFadeSec	= 1.5f;		// 倒れてから、画面が真っ黒になるまでの時間（秒）。真っ黒になったらゲームオーバーへ
	};

	// ブレイクの数値。「Enemy」タブで調整する
	// 【ブレイク】攻撃を当てるとゲージがたまり、満タンになると「ブレイク」して、しばらく攻撃が通りやすくなる
	//   ・普通の攻撃では少しずつしかたまらない（回避の直後・パリィの攻撃で大きくたまる予定。bonusGain）
	//   ・しばらく攻撃を当てないと、速めに減っていく
	//   ・ザコ・中ボス・ボスのどれをブレイクさせるかは、チェックボックスで選ぶ
	struct Battle
	{
		bool	breakEnable			= true;
		bool	normalBreak			= true;		// ザコもブレイクするか
		bool	midBossBreak		= true;		// 中ボスもブレイクするか
		float	breakMax			= 100.0f;	// ゲージの最大
		float	hitGain				= 1.2f;		// 普通の攻撃 1 回で、たまる量
		float	criticalRate		= 1.5f;		// クリティカルの時、たまる量に掛ける
		float	punisherRate		= 1.5f;		// パニッシャーの攻撃の時、たまる量に掛ける
		float	abilityRate			= 2.0f;		// ATB の技の時、たまる量に掛ける
		float	bonusGain			= 12.0f;	// 回避の直後・パリィの攻撃でたまる量（あとで作る）
		float	decayDelaySec		= 1.0f;		// 最後に当ててから、減り始めるまでの時間（秒）
		float	decayPerSec			= 15.0f;	// 1 秒に減る量
		float	breakSec			= 6.0f;		// ブレイクしている時間（秒）
		float	breakDamageRate		= 1.8f;		// ブレイク中に受けるダメージの倍率
	};

	// ---- プレイヤーの戦闘の数値（「Player」タブ。前は PlayerConstants.h の定数だった） ----
	// 【なぜ定数から変えたか】ゲームを動かしたまま、攻撃力などを変えて手ごたえを確かめるため
	struct PlayerTuning
	{
		// HP・攻撃
		int		maxHp					= 100;		// 最大 HP（ゲームを始めた時に使う。今の HP は「今のプレイヤー」で変える）
		int		attackDamage			= 3;		// 1 回当たった時のダメージ（モードの倍率を掛ける前）
		int		criticalAttackDamage	= 5;		// クリティカルの時のダメージ
		int		critRate				= 10;		// クリティカルになる確率（％）
		float	operatorDamageRate		= 0.6f;		// オペレーターのダメージの倍率
		float	punisherDamageRate		= 1.6f;		// パニッシャーのダメージの倍率
		float	abilityDamageRate		= 1.5f;		// ATB の技のダメージの倍率（モードの倍率はかからない）
		int		multiHitInterval		= 10;		// 同じ剣が同じ敵にもう一度当たれるまでのフレーム数
		int		abilityStunFrame		= 50;		// ATB の技が当たった敵が、のけぞり続けるフレーム数

		// モード
		float	operatorAttackSpeedRate	= 1.2f;		// オペレーターの攻撃の再生速度の倍率
		float	punisherAttackSpeedRate	= 0.8f;		// パニッシャーの攻撃の再生速度の倍率
		float	punisherMoveRate		= 0.6f;		// パニッシャーの移動の速さの倍率
		float	punisherTakenDamageRate	= 1.3f;		// パニッシャーの時に受けるダメージの倍率

		// 受けるダメージ
		int		damageInvincibleFrame	= 30;		// 攻撃を受けた後の無敵時間
		int		heavyDamage				= 20;		// これ以上のダメージで、大きくのけぞる
		float	evadeDamageRate			= 0.5f;		// 回避中に当たった時のダメージの倍率
		float	guardDamageRate			= 0.2f;		// ガード中に受けるダメージの倍率

		// ATB ゲージ（1 本 = 100）
		float	atbHitGain				= 2.5f;		// 攻撃を当てた時にたまる量
		float	operatorAtbHitRate		= 1.15f;	// オペレーターの時の倍率
		float	punisherAtbHitRate		= 0.75f;	// パニッシャーの時の倍率
		float	atbCriticalRate			= 1.25f;	// クリティカルの時の倍率
		float	atbEvadeGain			= 5.0f;		// 回避した時にたまる量

		// ガード
		float	guardDurabilityMax		= 100.0f;	// 盾の耐久値の最大
		float	guardCostPerDamage		= 3.0f;		// 受け止めたダメージ × これだけ減る
		float	guardCriticalCostRate	= 1.5f;		// クリティカルの攻撃は多く減る
		int		guardRegenDelay			= 120;		// 最後に受け止めてから、回復し始めるまでのフレーム数
		float	guardRegenSpeed			= 0.15f;	// 1 フレームに回復する量
		int		guardBreakFrame			= 120;		// 崩された後、ガードできない時間

		// ロックオン
		float	lockOnRange				= 10.0f;	// この距離に近づいた敵に、自動でロックオンする
	};

	// ---- 敵の数値（「Enemy」タブ。前は Enemy.cpp・EnemyState.cpp の定数だった） ----
	// 敵の種類ごとの数値（敵を作った時に使う。「今いる敵に反映」ボタンで、今いる敵にも掛け直せる）
	struct EnemyType
	{
		int		hp;				// 最大 HP
		float	scale;			// 体の大きさ
		float	searchRange;	// プレイヤーに気づく距離
		float	attackRange;	// 攻撃を始める距離
		int		jumpAttackCT;	// 飛びかかり攻撃を使った後、次に使えるまでのフレーム数
	};
	struct EnemyTuning
	{
		EnemyType	normal	= { 130, 1.25f,  8.0f, 1.2f, 60 * 10 };	// ザコ
		EnemyType	midBoss	= { 330, 1.9f,  12.0f, 2.4f, 60 * 7 };	// 中ボス
		EnemyType	boss	= { 560, 2.6f,  16.0f, 3.5f, 60 * 6 };	// ボス

		// 攻撃
		int		attackDamage			= 5;		// 腕の攻撃のダメージ
		int		criticalAttackDamage	= 10;		// 腕の攻撃のダメージ（クリティカル。今は使っていない）
		int		smashDamage				= 20;		// 飛びかかりの着地のダメージ
		int		attackRecoverFrame		= 50;		// 攻撃の後のすき（ザコ・中ボス）
		int		bossAttackRecoverFrame	= 35;		// 攻撃の後のすき（ボス）
		int		attackRecoverRandom		= 20;		// すきのばらつき
		int		jumpAttackRecoverFrame	= 50;		// 飛びかかり攻撃の後のすき
		int		jumpAttackChance		= 3;		// 飛びかかれる距離にいる時、1 フレームに使う確率（％）
		float	jumpAttackMinDist		= 4.0f;		// プレイヤーがこれより遠く、
		float	jumpAttackMaxDist		= 14.0f;	// これより近い時に飛びかかる

		// 動き
		float	chaseSpeed				= 0.1f;		// 追いかける速さ（1 フレームに進む m）

		// ノックバック（殴られた時に後ろへすべる）
		float	knockbackSpeed			= 0.12f;	// 普通の攻撃
		float	knockbackCriticalRate	= 1.6f;		// クリティカルの倍率
		float	knockbackDecay			= 0.78f;	// 1 フレームごとに掛ける（小さいほど早く止まる）
		float	knockbackArmorRate		= 0.3f;		// 攻撃中（スーパーアーマー）の倍率
		float	knockbackStunRate		= 0.5f;		// ATB の技を受けている間の倍率
		float	knockbackMidBossRate	= 0.5f;		// 中ボスの倍率
		float	knockbackBossRate		= 0.25f;	// ボスの倍率
	};

	static StageSettings& Instance()
	{
		static StageSettings instance;
		return instance;
	}

	Fog&				WorkFog()			{ return m_fog; }
	const Fog&			GetFog() const		{ return m_fog; }
	const GroundMist&	GetGroundMist() const { return m_groundMist; }
	const LowHpScreen&	GetLowHpScreen() const { return m_lowHp; }
	const Battle&		GetBattle() const	{ return m_battle; }
	const PlayerTuning&	GetPlayer() const	{ return m_player; }
	const EnemyTuning&	GetEnemy() const	{ return m_enemy; }

	// 霧の設定を、シェーダー（KdAmbientController）に渡す（ゲームの画面で毎フレーム呼ぶ）
	void ApplyFog() const;

	// オブジェクトエディタのウィンドウ（KdDebugGUI がデバッグ表示中に呼ぶ）
	void DrawEditor();

	// 保存・読み込み（ファイルが無い時は、今の値のまま）
	bool Save(const std::string& path = kDefaultPath) const;
	bool Load(const std::string& path = kDefaultPath);

	// 初期値に戻す（Player タブの剣の持ち方などは戻さない）
	void ResetAll()
	{
		m_fog = Fog(); m_groundMist = GroundMist(); m_lowHp = LowHpScreen();
		m_battle = Battle(); m_player = PlayerTuning(); m_enemy = EnemyTuning();
	}

	static inline const std::string kDefaultPath = "Asset/Data/StageSettings.json";
	static inline const std::string kPlayerParamPath = "Asset/Data/PlayerParams.txt";	// 回避の速さ（PlayerParamManager）
	static inline const std::string kWeaponParamPath = "Asset/Data/WeaponParams.json";	// 剣の持ち方（PlayerParamManager）

private:

	void DrawFogEditor();
	void DrawGroundMistEditor();
	void DrawLowHpEditor();
	void DrawBattleEditor();
	void DrawPlayerEditor();
	void DrawLivePlayerEditor();	// 今のプレイヤー（HP・ATB を直接変える）
	void DrawLiveEnemyEditor();		// 今いる敵の一覧（HP・ブレイクを直接変える）

	Fog			m_fog;
	GroundMist	m_groundMist;
	LowHpScreen	m_lowHp;
	Battle		m_battle;
	PlayerTuning m_player;
	EnemyTuning	m_enemy;
	std::string	m_message;		// 保存・読み込みの結果（エディタに表示する）

	StageSettings() {}
};

// 調整できる戦闘の数値を、短く読むための関数（例：PlayerTune().attackDamage）
inline const StageSettings::PlayerTuning&	PlayerTune()	{ return StageSettings::Instance().GetPlayer(); }
inline const StageSettings::EnemyTuning&	EnemyTune()		{ return StageSettings::Instance().GetEnemy(); }
