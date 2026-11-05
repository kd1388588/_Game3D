#pragma once

// ============================================================
// ステージの見た目の設定（ステージエディタで調整する値）
// ・F8 のデバッグ表示を出すと「Stage Editor」のウィンドウが出て、ゲームを動かしたまま調整できる
// ・「保存」で Asset/Data/StageSettings.json に書き出し、次にゲームを始めた時に読み込む
// ・今は「霧」の項目だけ（距離の霧・高さの霧・地面の霧）。項目はここに足していく
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
	struct LowHpScreen
	{
		bool	enable			= true;
		float	startRate		= 0.3f;		// この HP の割合（0～1）より少なくなると始まる
		float	grayscale		= 1.0f;		// HP 0 の時の白黒の強さ（1 で完全な白黒）
		float	darken			= 0.35f;	// HP 0 の時に、全体を暗くする強さ
		float	vignette		= 0.97f;	// HP 0 の時の周辺減光の強さ（1 でふちが真っ黒）
		float	vignetteStart	= 0.25f;	// 周辺減光が始まる位置（0：真ん中、1：上下左右のふち）
		float	followRate		= 0.05f;	// HP が変わった時に、画面がその強さに近づく速さ（1フレームに近づく割合。小さいほどゆっくり）
		float	blinkRate		= 0.3f;		// HP ゲージが赤く点滅し始める HP の割合
		float	blinkSpeed		= 0.12f;	// 点滅の速さ
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

	// 霧の設定を、シェーダー（KdAmbientController）に渡す（ゲームの画面で毎フレーム呼ぶ）
	void ApplyFog() const;

	// ステージエディタのウィンドウ（KdDebugGUI がデバッグ表示中に呼ぶ）
	void DrawEditor();

	// 保存・読み込み（ファイルが無い時は、今の値のまま）
	bool Save(const std::string& path = kDefaultPath) const;
	bool Load(const std::string& path = kDefaultPath);

	// すべて初期値に戻す
	void ResetAll() { m_fog = Fog(); m_groundMist = GroundMist(); m_lowHp = LowHpScreen(); }

	static inline const std::string kDefaultPath = "Asset/Data/StageSettings.json";

private:

	void DrawFogEditor();
	void DrawGroundMistEditor();
	void DrawLowHpEditor();

	Fog			m_fog;
	GroundMist	m_groundMist;
	LowHpScreen	m_lowHp;
	std::string	m_message;		// 保存・読み込みの結果（エディタに表示する）

	StageSettings() {}
};
