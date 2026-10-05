#pragma once
#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>

struct AttackParam
{
	float hitStartFrame = 10.0f;
	float hitEndFrame = 25.0f;
	float cancelFrame = 30.0f;
};

class PlayerParamManager
{
public:
	static PlayerParamManager& Instance()
	{
		static PlayerParamManager instance;
		return instance;
	}

	void Load(const std::string& filepath);
	void Save(const std::string& filepath);

	void LoadWeaponParams(const std::string& filepath);
	void SaveWeaponParams(const std::string& filepath);

	// 攻撃パラメータの取得
	AttackParam GetAttackParam(const std::string& animName)
	{
		if (m_attackParams.find(animName) == m_attackParams.end()) {
			m_attackParams[animName] = AttackParam();
		}
		return m_attackParams[animName];
	}

	// GUI調整用にMapを渡す
	std::unordered_map<std::string, AttackParam>& GetAttackMap() { return m_attackParams; }

	float m_evadeSpeed = 0.4f;
	float m_dashSpeed = 0.5f;
	float m_attackRootScale = 1.0f;

private:
	PlayerParamManager() {}
	~PlayerParamManager() {}

	std::unordered_map<std::string, AttackParam> m_attackParams;
};