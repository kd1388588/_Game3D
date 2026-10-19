#include "SoundPlayer.h"
#include <filesystem>

std::shared_ptr<KdSoundInstance>	SoundPlayer::s_bgm = nullptr;
std::string							SoundPlayer::s_bgmName;

void SoundPlayer::PlayBGM(const std::string& fileName, bool loop, float volume)
{
	// 同じ BGM が流れている時はそのまま（シーンをやり直した時に、曲が頭に戻らないように）
	if (s_bgm && s_bgmName == fileName && s_bgm->IsPlaying()) return;

	StopBGM();
	if (!Exists(fileName)) return;

	s_bgm = KdAudioManager::Instance().Play(fileName, loop);
	if (s_bgm)
	{
		s_bgm->SetVolume(volume);
		s_bgmName = fileName;
	}
}

void SoundPlayer::StopBGM()
{
	if (s_bgm)
	{
		s_bgm->Stop();
		s_bgm = nullptr;
	}
	s_bgmName.clear();
}

void SoundPlayer::PlaySE(const std::string& fileName, float volume)
{
	if (!Exists(fileName)) return;

	if (auto se = KdAudioManager::Instance().Play(fileName, false))
	{
		se->SetVolume(volume);
	}
}

bool SoundPlayer::Exists(const std::string& fileName)
{
	// 毎回ファイルを調べないよう、結果を覚えておく
	static std::unordered_map<std::string, bool> s_existsCache;

	auto it = s_existsCache.find(fileName);
	if (it != s_existsCache.end()) return it->second;

	bool exists = std::filesystem::exists(fileName);
	if (!exists)
	{
		// フレームワーク側はファイルが無いとデバッグ版で止まるので、ここで弾いてログを出す
		OutputDebugStringA(("【サウンド読込失敗 (ファイルなし)】: " + fileName + "\n").c_str());
	}

	s_existsCache[fileName] = exists;
	return exists;
}
