#pragma once

// ============================================================
// 使用する音のファイル名
// ・ゲームの音（DirectXTK の SoundEffect）は wav しか読めないので、Asset/Music の mp3 を
//   Tools/MakeSoundAssets.py で wav に変換して Asset/Sounds に置いている
// ・空文字 "" にしたもの・ファイルが無いものは鳴らさない
// ============================================================
namespace SoundName
{
	// BGM
	inline const std::string TitleBGM			= "Asset/Sounds/BGM/Break_Down.wav";	// タイトル画面
	inline const std::string GameBGM			= "Asset/Sounds/BGM/Deckrail.wav";		// ゲーム画面
	inline const std::string GameClearBGM		= "Asset/Sounds/BGM/GameClear.wav";		// GAME Clear 画面
	inline const std::string GameOverBGM		= "Asset/Sounds/BGM/GameOver.wav";		// GAME OVER 画面

	// プレイヤーの攻撃が敵に当たった
	inline const std::string Slash				= "Asset/Sounds/SE/Slash.wav";

	// 敵の攻撃が当たった（プレイヤーが受けた・ガードした・ボスの叩きつけ）
	inline const std::string HeavyHit			= "Asset/Sounds/SE/HeavyHit.wav";

	// 足音（足が地面に着くたびに、4つの中から順番に鳴らす。毎回同じ音だと機械的に聞こえるため）
	inline const std::string Footsteps[]		=
	{
		"Asset/Sounds/SE/Footstep1.wav",
		"Asset/Sounds/SE/Footstep2.wav",
		"Asset/Sounds/SE/Footstep3.wav",
		"Asset/Sounds/SE/Footstep4.wav",
	};
}

// ============================================================
// 音を鳴らす補助クラス
// ・BGM は1曲だけ持ち、ループで流す（シーンを切り替えると止まる）
// ・効果音は鳴らしっぱなし（KdAudioManager が、鳴り終わったら片付ける）
// ============================================================
class SoundPlayer
{
public:

	// BGM をループで流す（同じ曲が流れている時は、最初からにしない）
	static void PlayBGM(const std::string& fileName, float volume = kBGMVolume)
	{
		if (s_bgm && s_bgmName == fileName && s_bgm->IsPlaying()) return;

		StopBGM();
		if (!Exists(fileName)) return;

		s_bgm = KdAudioManager::Instance().Play(fileName, true);
		s_bgmName = fileName;
		if (s_bgm) s_bgm->SetVolume(volume);
	}

	static void StopBGM()
	{
		if (s_bgm) s_bgm->Stop();
		s_bgm = nullptr;
		s_bgmName.clear();
	}

	// 効果音を1回鳴らす
	// pitchRandom：音の高さをこの幅でばらつかせる（同じ音の繰り返しが機械的に聞こえないように）
	static void PlaySE(const std::string& fileName, float volume = 1.0f, float pitch = 0.0f, float pitchRandom = 0.0f)
	{
		if (!Exists(fileName)) return;

		std::shared_ptr<KdSoundInstance> se = KdAudioManager::Instance().Play(fileName, false);
		if (!se) return;

		if (pitchRandom > 0.0f)
		{
			std::uniform_real_distribution<float> dist(-pitchRandom, pitchRandom);
			pitch += dist(s_random);
		}
		se->SetVolume(volume * s_seVolume);
		se->SetPitch(std::clamp(pitch, -1.0f, 1.0f));
	}

	// 足音を1歩鳴らす（4つの足音を順番に使う）
	static void PlayFootstep(float volume)
	{
		constexpr int kCount = static_cast<int>(std::size(SoundName::Footsteps));
		s_footstepIndex = (s_footstepIndex + 1) % kCount;
		PlaySE(SoundName::Footsteps[s_footstepIndex], volume, 0.0f, 0.08f);
	}

	// すべての音を止める（シーンの切り替え時）
	static void StopAll()
	{
		StopBGM();
		KdAudioManager::Instance().StopAllSound();
	}

private:

	static constexpr float kBGMVolume = 0.4f;	// BGM の大きさ（効果音が聞こえるように少し小さく）

	// ファイルがあるか（無いファイルを読むとフレームワークが止まる（assert）ので、先に調べる）
	static bool Exists(const std::string& fileName)
	{
		if (fileName.empty()) return false;

		auto it = s_exists.find(fileName);
		if (it != s_exists.end()) return it->second;

		bool exists = std::filesystem::exists(fileName);
		s_exists[fileName] = exists;
		return exists;
	}

	inline static std::shared_ptr<KdSoundInstance>	s_bgm;
	inline static std::string						s_bgmName;
	inline static float								s_seVolume = 0.8f;
	inline static int								s_footstepIndex = 0;
	inline static std::mt19937						s_random{ std::random_device{}() };
	inline static std::unordered_map<std::string, bool>	s_exists;
};
