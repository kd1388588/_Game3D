#pragma once

// ============================================================
// エフェクトの出し方の設定（エフェクトエディタで調整する値）
// ・Effekseer のエフェクトのファイル（Asset/Data/Effect/*.efkefc）ごとに、
//   大きさ・再生速度・高さのずれ・色（プレイヤーのモードの色）を持つ
// ・EffectPlayer でエフェクトを出す時に、ここの値を自動でかける（呼ぶ側は何もしなくてよい）
// ・F8 のデバッグ表示を出すと「Effect Editor」のウィンドウが出て、ゲームを動かしたまま調整できる
// ・「保存」で Asset/Data/EffectSettings.json に書き出し、次にゲームを始めた時に読み込む
// ・「Effekseer と同期」を開始すると、Effekseer エディタで編集したエフェクトが、動いているゲームにすぐ反映される
// ============================================================
class EffectSettings
{
public:

	// エフェクト1つ分の設定
	struct Entry
	{
		float		scale			= 1.0f;		// 大きさの倍率（呼ぶ側が決めた大きさに掛ける）
		float		speed			= 1.0f;		// 再生速度の倍率
		float		heightOffset	= 0.0f;		// 出す位置を上下にずらす（m）
		bool		useModeColor	= false;	// プレイヤーのモードで色を変えるか
		Math::Color	color			= { 1.0f, 1.0f, 1.0f, 1.0f };	// 色（useModeColor が false の時）
		Math::Color	operatorColor	= { 1.0f, 1.0f, 1.0f, 1.0f };	// オペレーターの時の色
		Math::Color	punisherColor	= { 1.0f, 1.0f, 1.0f, 1.0f };	// パニッシャーの時の色
	};

	static EffectSettings& Instance()
	{
		static EffectSettings instance;
		return instance;
	}

	// そのエフェクトの設定（まだ無ければ初期値を作る）
	const Entry& Get(const std::string& fileName);

	// 今かける色（モードの色を使うエフェクトなら、今のモードの色）
	Math::Color GetColor(const std::string& fileName);

	// プレイヤーの今のモード（Player が毎フレーム・切り替えた時に教える）
	void SetPlayerPunisher(bool isPunisher) { m_isPlayerPunisher = isPunisher; }

	// エフェクトエディタのウィンドウ（KdDebugGUI がデバッグ表示中に呼ぶ）
	void DrawEditor();

	// 保存・読み込み（ファイルが無い時は、今の値のまま）
	bool Save(const std::string& path = kDefaultPath) const;
	bool Load(const std::string& path = kDefaultPath);

	// ゲームの始まりに呼ぶ（設定を読み込み、「起動時に同期を始める」なら同期を始める）
	void OnGameStart();

	static inline const std::string kDefaultPath = "Asset/Data/EffectSettings.json";

private:

	// ファイル名から、初期値を決める（プレイヤーの斬撃はモードの色、敵の爪は赤 など）
	static Entry MakeDefault(const std::string& fileName);

	// Asset/Data/Effect の中の .efkefc を探して、一覧を作る
	void RefreshFileList();

	// 試し撃ち（カメラの前にエフェクトを出す）
	void PlayPreview(const std::string& fileName);

	void DrawSyncEditor();
	void DrawEntryEditor(const std::string& fileName, Entry& entry);

	std::map<std::string, Entry>	m_entries;
	std::vector<std::string>		m_fileList;
	std::string						m_selected;
	std::string						m_message;

	bool	m_isPlayerPunisher	= false;

	// 試し撃ち
	int		m_previewMode		= 0;		// 0：今のモード、1：オペレーター、2：パニッシャー
	bool	m_previewRepeat		= false;	// 続けて出す
	float	m_previewInterval	= 1.5f;		// 続けて出す間隔（秒）
	float	m_previewTimer		= 0.0f;
	float	m_previewDistance	= 4.0f;		// カメラから何 m 前に出すか
	float	m_previewScale		= 0.5f;		// 試し撃ちの大きさ（ゲームで呼ぶ時の大きさの代わり）

	// Effekseer との同期
	bool	m_syncAutoStart		= false;	// ゲームを始めた時に、同期を始めるか
	int		m_syncPort			= 60000;	// Effekseer エディタの「ネットワーク」と同じ番号にする

	EffectSettings() {}
};
