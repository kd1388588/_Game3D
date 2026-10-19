#pragma once

// BGM・SE（効果音）のファイル名（Tools/MakeSound.py で作成）
namespace SoundName
{
	// BGM
	inline const std::string BgmTitle		= "Asset/Sound/BGM/Title.wav";
	inline const std::string BgmBattle		= "Asset/Sound/BGM/Battle.wav";
	inline const std::string BgmClear		= "Asset/Sound/BGM/Clear.wav";		// 1回だけ流す
	inline const std::string BgmGameOver	= "Asset/Sound/BGM/GameOver.wav";	// 1回だけ流す

	// SE
	inline const std::string Slash			= "Asset/Sound/SE/Slash.wav";
	inline const std::string Hit			= "Asset/Sound/SE/Hit.wav";
	inline const std::string Critical		= "Asset/Sound/SE/Critical.wav";
	inline const std::string Evade			= "Asset/Sound/SE/Evade.wav";
	inline const std::string Jump			= "Asset/Sound/SE/Jump.wav";
	inline const std::string Land			= "Asset/Sound/SE/Land.wav";
	inline const std::string Guard			= "Asset/Sound/SE/Guard.wav";
	inline const std::string GuardBreak		= "Asset/Sound/SE/GuardBreak.wav";
	inline const std::string ModeChange		= "Asset/Sound/SE/ModeChange.wav";
	inline const std::string Ability		= "Asset/Sound/SE/Ability.wav";
	inline const std::string EnemyDeath		= "Asset/Sound/SE/EnemyDeath.wav";
	inline const std::string BossSmash		= "Asset/Sound/SE/BossSmash.wav";
	inline const std::string PlayerDamage	= "Asset/Sound/SE/PlayerDamage.wav";
	inline const std::string MenuSelect		= "Asset/Sound/SE/MenuSelect.wav";
	inline const std::string MenuDecide		= "Asset/Sound/SE/MenuDecide.wav";
	inline const std::string MenuOpen		= "Asset/Sound/SE/MenuOpen.wav";
}

// BGM・SE を鳴らす（ファイルが無い時は何もしない）
class SoundPlayer
{
public:
	// BGM を流す（前の BGM は止める。同じ BGM が流れている時は、最初からやり直さない）
	static void PlayBGM(const std::string& fileName, bool loop = true, float volume = kBgmVolume);
	static void StopBGM();

	// SE を鳴らす
	static void PlaySE(const std::string& fileName, float volume = kSeVolume);

	// 全体の音量（0～1）
	static constexpr float kBgmVolume	= 0.5f;
	static constexpr float kSeVolume	= 0.8f;

private:
	static bool Exists(const std::string& fileName);

	static std::shared_ptr<KdSoundInstance>	s_bgm;
	static std::string						s_bgmName;
};
