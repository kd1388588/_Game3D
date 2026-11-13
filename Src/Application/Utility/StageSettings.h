#pragma once

// ============================================================
// ステージの見た目の設定（ステージエディタで調整する値）
// ・F8 のデバッグ表示を出すと「Stage Editor」のウィンドウが出て、ゲームを動かしたまま調整できる
// ・「保存」で Asset/Data/StageSettings.json に書き出し、次にゲームを始めた時に読み込む
// ・今は「霧」の項目だけ（距離の霧・高さの霧・地面の霧）。項目はここに足していく
// ・「画面」タブには、HP が少ない時の画面（白黒・暗さ・周辺減光）の設定もある
// ・「戦闘」タブには、敵の HP の倍率と、ボスのブレイクの設定がある
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

	// 戦闘の数値（敵の HP の倍率・ボスのブレイク）。「戦闘」タブで調整する
	// 【ブレイク】攻撃を当てるとゲージがたまり、満タンになると「ブレイク」して、しばらく攻撃が通りやすくなる
	//   ・普通の攻撃では少しずつしかたまらない（回避の直後・パリィの攻撃で大きくたまる予定。bonusGain）
	//   ・しばらく攻撃を当てないと、速めに減っていく
	//   ・ザコ・中ボス・ボスのどれをブレイクさせるかは、チェックボックスで選ぶ
	struct Battle
	{
		// 敵の HP の倍率（敵を作った時に掛ける。「今いる敵に反映」ボタンで、今いる敵にも掛け直せる）
		float	normalHpRate		= 1.3f;		// ザコ（元の HP 100）
		float	midBossHpRate		= 1.5f;		// 中ボス（元の HP 220）
		float	bossHpRate			= 1.6f;		// ボス（元の HP 350）

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

	// 霧の設定を、シェーダー（KdAmbientController）に渡す（ゲームの画面で毎フレーム呼ぶ）
	void ApplyFog() const;

	// ステージエディタのウィンドウ（KdDebugGUI がデバッグ表示中に呼ぶ）
	void DrawEditor();

	// 保存・読み込み（ファイルが無い時は、今の値のまま）
	bool Save(const std::string& path = kDefaultPath) const;
	bool Load(const std::string& path = kDefaultPath);

	// すべて初期値に戻す
	void ResetAll() { m_fog = Fog(); m_groundMist = GroundMist(); m_lowHp = LowHpScreen(); m_battle = Battle(); }

	static inline const std::string kDefaultPath = "Asset/Data/StageSettings.json";

private:

	void DrawFogEditor();
	void DrawGroundMistEditor();
	void DrawLowHpEditor();
	void DrawBattleEditor();

	Fog			m_fog;
	GroundMist	m_groundMist;
	LowHpScreen	m_lowHp;
	Battle		m_battle;
	std::string	m_message;		// 保存・読み込みの結果（エディタに表示する）

	StageSettings() {}
};
