#include "PlayerParamManager.h"
#include "Player.h" // Playerの静的変数にアクセスするため追加

void PlayerParamManager::Load(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	std::string line;

	if (std::getline(file, line))
	{
		std::istringstream iss(line);
		iss >> m_evadeSpeed >> m_dashSpeed >> m_attackRootScale;
	}

	while (std::getline(file, line))
	{
		std::istringstream iss(line);
		std::string name;
		float start, end, cancel;
		if (iss >> name >> start >> end >> cancel)
		{
			m_attackParams[name] = { start, end, cancel };
		}
	}
}

void PlayerParamManager::Save(const std::string& filepath)
{
	std::ofstream file(filepath);
	if (!file.is_open()) return;

	file << m_evadeSpeed << " " << m_dashSpeed << " " << m_attackRootScale << "\n";

	for (const auto& pair : m_attackParams)
	{
		file << pair.first << " "
			<< pair.second.hitStartFrame << " "
			<< pair.second.hitEndFrame << " "
			<< pair.second.cancelFrame << "\n";
	}
}

const std::vector<PlayerParamManager::WeaponParamEntry>& PlayerParamManager::GetWeaponParamEntries()
{
	static const std::vector<WeaponParamEntry> entries =
	{
		{ "WeaponR",	&Player::s_weaponPosR,		&Player::s_weaponRotR },
		{ "WeaponL",	&Player::s_weaponPosL,		&Player::s_weaponRotL },
		{ "SheathedR",	&Player::s_sheathedPosR,	&Player::s_sheathedRotR },
		{ "SheathedL",	&Player::s_sheathedPosL,	&Player::s_sheathedRotL },
	};
	return entries;
}

bool PlayerParamManager::ParseVec3(const std::string& line, Math::Vector3& out)
{
	size_t start = line.find('[');
	size_t end = line.find(']');
	if (start == std::string::npos || end == std::string::npos) return false;

	std::string vals = line.substr(start + 1, end - start - 1);
	for (char& c : vals) { if (c == ',') c = ' '; } // カンマを空白に変換

	std::istringstream iss(vals);
	float x, y, z;
	if (!(iss >> x >> y >> z)) return false;

	out = { x, y, z };
	return true;
}

// ====================================================================
// 武器座標のJSON読み込み処理（標準ライブラリ版簡易JSONパーサー）
// ====================================================================
void PlayerParamManager::LoadWeaponParams(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	const WeaponParamEntry* pCurrent = nullptr;
	std::string line;

	while (std::getline(file, line))
	{
		// どの武器のブロックかを判定
		for (const auto& entry : GetWeaponParamEntries())
		{
			if (line.find("\"" + std::string(entry.key) + "\"") != std::string::npos)
			{
				pCurrent = &entry;
				break;
			}
		}

		if (!pCurrent) continue;

		if (line.find("\"Pos\"") != std::string::npos)
		{
			ParseVec3(line, *pCurrent->pos);
		}
		else if (line.find("\"Rot\"") != std::string::npos)
		{
			ParseVec3(line, *pCurrent->rot);
		}
	}
}

// ====================================================================
// 武器座標のJSON保存処理
// ====================================================================
void PlayerParamManager::SaveWeaponParams(const std::string& filepath)
{
	std::ofstream file(filepath);
	if (!file.is_open()) return;

	// 綺麗にフォーマットして出力するラムダ式
	auto writeVec3 = [&file](const std::string& name, const Math::Vector3& v, bool isLast)
	{
		file << "\t\t\"" << name << "\": [" << v.x << ", " << v.y << ", " << v.z << "]" << (isLast ? "\n" : ",\n");
	};

	const auto& entries = GetWeaponParamEntries();

	file << "{\n";
	for (size_t i = 0; i < entries.size(); ++i)
	{
		bool isLastEntry = (i == entries.size() - 1);

		file << "\t\"" << entries[i].key << "\": {\n";
		writeVec3("Pos", *entries[i].pos, false);
		writeVec3("Rot", *entries[i].rot, true);
		file << (isLastEntry ? "\t}\n" : "\t},\n");
	}
	file << "}\n";
}
