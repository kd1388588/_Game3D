#include "EffectPlayer.h"
#include <filesystem>

std::shared_ptr<KdEffekseerObject> EffectPlayer::Play(const std::string& fileName, const Math::Vector3& pos,
	float scale, float speed)
{
	if (!Exists(fileName)) return nullptr;

	return KdEffekseerManager::GetInstance().Play(fileName, pos, scale, speed, false).lock();
}

std::shared_ptr<KdEffekseerObject> EffectPlayer::Play(const std::string& fileName, const Math::Matrix& mWorld,
	float speed)
{
	std::shared_ptr<KdEffekseerObject> effect = Play(fileName, mWorld.Translation(), 1.0f, speed);
	if (effect)
	{
		// 位置だけでなく向き・大きさも行列で上書きする
		effect->SetWorldMatrix(mWorld);
	}
	return effect;
}

void EffectPlayer::Stop(std::shared_ptr<KdEffekseerObject>& effect)
{
	if (effect)
	{
		effect->StopEffect();
		effect = nullptr;
	}
}

void EffectPlayer::StopAll()
{
	KdEffekseerManager::GetInstance().StopAllEffect();
}

bool EffectPlayer::Exists(const std::string& fileName)
{
	if (fileName.empty()) return false;

	// 毎回ファイルを調べないよう、結果を覚えておく
	static std::unordered_map<std::string, bool> s_existsCache;

	auto it = s_existsCache.find(fileName);
	if (it != s_existsCache.end()) return it->second;

	bool exists = std::filesystem::exists(std::string(EffekseerPath) + fileName);
	if (!exists)
	{
		// フレームワーク側はファイルが無いとデバッグ版で止まるので、ここで弾いてログを出す
		OutputDebugStringA(("【エフェクト読込失敗 (ファイルなし)】: " + std::string(EffekseerPath) + fileName + "\n").c_str());
	}

	s_existsCache[fileName] = exists;
	return exists;
}
