#include "EffectPlayer.h"
#include "../../Utility/EffectSettings.h"
#include <filesystem>

// ※どの Play も、エフェクトエディタの設定（EffectSettings：大きさ・再生速度・高さのずれ・色）を自動でかける

std::shared_ptr<KdEffekseerObject> EffectPlayer::PlayRaw(const std::string& fileName, const Math::Vector3& pos,
	float scale, float speed)
{
	if (!s_isEnabled) return nullptr;
	if (!Exists(fileName)) return nullptr;

	std::shared_ptr<KdEffekseerObject> effect = KdEffekseerManager::GetInstance().Play(fileName, pos, scale, speed, false).lock();
	if (effect)
	{
		// 色（白なら元の色のまま）
		Math::Color color = EffectSettings::Instance().GetColor(fileName);
		if (color.x != 1.0f || color.y != 1.0f || color.z != 1.0f || color.w != 1.0f)
		{
			effect->SetColor(color);
		}
	}
	return effect;
}

std::shared_ptr<KdEffekseerObject> EffectPlayer::Play(const std::string& fileName, const Math::Vector3& pos,
	float scale, float speed)
{
	const EffectSettings::Entry& setting = EffectSettings::Instance().Get(fileName);
	Math::Vector3 p = pos + Math::Vector3(0.0f, setting.heightOffset, 0.0f);
	return PlayRaw(fileName, p, scale * setting.scale, speed * setting.speed);
}

std::shared_ptr<KdEffekseerObject> EffectPlayer::Play(const std::string& fileName, const Math::Matrix& mWorld,
	float speed)
{
	// 大きさの倍率は行列の前に掛け、高さのずれは位置に足す
	const EffectSettings::Entry& setting = EffectSettings::Instance().Get(fileName);
	Math::Matrix m = Math::Matrix::CreateScale(setting.scale) * mWorld;
	m.Translation(m.Translation() + Math::Vector3(0.0f, setting.heightOffset, 0.0f));

	std::shared_ptr<KdEffekseerObject> effect = PlayRaw(fileName, m.Translation(), 1.0f, speed * setting.speed);
	if (effect)
	{
		// 位置だけでなく向き・大きさも行列で上書きする
		effect->SetWorldMatrix(m);
	}
	return effect;
}

std::shared_ptr<KdEffekseerObject> EffectPlayer::PlayFacingCamera(const std::string& fileName, const Math::Vector3& pos,
	float scale, const Math::Vector3& screenDir)
{
	// screenDir を画面の上の向きにして、その角度だけ Z で回す
	Math::Matrix camRot = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	float angle = 0.0f;
	float dx = screenDir.Dot(camRot.Right());
	float dy = screenDir.Dot(camRot.Up());
	if (dx * dx + dy * dy > 0.000001f)
	{
		angle = std::atan2(dy, dx);
	}
	return PlayFacingCameraRotZ(fileName, pos, scale, angle);
}

std::shared_ptr<KdEffekseerObject> EffectPlayer::PlayFacingCameraRotZ(const std::string& fileName, const Math::Vector3& pos,
	float scale, float rotZ)
{
	// カメラの向き（ビュー行列の逆）。X：画面の右、Y：画面の上、Z：画面の奥
	Math::Matrix camRot = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	camRot.Translation(Math::Vector3::Zero);

	Math::Matrix mWorld = Math::Matrix::CreateScale(scale)
		* Math::Matrix::CreateRotationZ(rotZ)
		* camRot
		* Math::Matrix::CreateTranslation(pos);
	return Play(fileName, mWorld);
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
