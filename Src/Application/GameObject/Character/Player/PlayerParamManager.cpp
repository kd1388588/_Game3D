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

// ====================================================================
// 武器座標のJSON読み込み処理（標準ライブラリ版簡易JSONパーサー）
// ====================================================================
void PlayerParamManager::LoadWeaponParams(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) return;

	std::string line;
	std::string currentKey = "";

	while (std::getline(file, line))
	{
		if (line.find("\"WeaponR\"") != std::string::npos) currentKey = "WeaponR";
		else if (line.find("\"WeaponL\"") != std::string::npos) currentKey = "WeaponL";
		else if (line.find("\"SheathedR\"") != std::string::npos) currentKey = "SheathedR";
		else if (line.find("\"SheathedL\"") != std::string::npos) currentKey = "SheathedL";

		if (line.find("\"Pos\"") != std::string::npos)
		{
			size_t start = line.find('[');
			size_t end = line.find(']');
			if (start != std::string::npos && end != std::string::npos)
			{
				std::string vals = line.substr(start + 1, end - start - 1);
				for (char& c : vals) { if (c == ',') c = ' '; } // カンマを空白に変換
				std::istringstream iss(vals);
				float x, y, z;
				if (iss >> x >> y >> z)
				{
					if (currentKey == "WeaponR") Player::s_weaponPosR = { x, y, z };
					else if (currentKey == "WeaponL") Player::s_weaponPosL = { x, y, z };
					else if (currentKey == "SheathedR") Player::s_sheathedPosR = { x, y, z };
					else if (currentKey == "SheathedL") Player::s_sheathedPosL = { x, y, z };
				}
			}
		}
		else if (line.find("\"Rot\"") != std::string::npos)
		{
			size_t start = line.find('[');
			size_t end = line.find(']');
			if (start != std::string::npos && end != std::string::npos)
			{
				std::string vals = line.substr(start + 1, end - start - 1);
				for (char& c : vals) { if (c == ',') c = ' '; } // カンマを空白に変換
				std::istringstream iss(vals);
				float x, y, z;
				if (iss >> x >> y >> z)
				{
					if (currentKey == "WeaponR") Player::s_weaponRotR = { x, y, z };
					else if (currentKey == "WeaponL") Player::s_weaponRotL = { x, y, z };
					else if (currentKey == "SheathedR") Player::s_sheathedRotR = { x, y, z };
					else if (currentKey == "SheathedL") Player::s_sheathedRotL = { x, y, z };
				}
			}
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

	file << "{\n";

	// 綺麗にフォーマットして出力するラムダ式
	auto writeVec3 = [&file](const std::string& name, const Math::Vector3& v, bool isLast) {
		file << "\t\t\"" << name << "\": [" << v.x << ", " << v.y << ", " << v.z << "]" << (isLast ? "\n" : ",\n");
		};

	file << "\t\"WeaponR\": {\n";
	writeVec3("Pos", Player::s_weaponPosR, false);
	writeVec3("Rot", Player::s_weaponRotR, true);
	file << "\t},\n";

	file << "\t\"WeaponL\": {\n";
	writeVec3("Pos", Player::s_weaponPosL, false);
	writeVec3("Rot", Player::s_weaponRotL, true);
	file << "\t},\n";

	file << "\t\"SheathedR\": {\n";
	writeVec3("Pos", Player::s_sheathedPosR, false);
	writeVec3("Rot", Player::s_sheathedRotR, true);
	file << "\t},\n";

	file << "\t\"SheathedL\": {\n";
	writeVec3("Pos", Player::s_sheathedPosL, false);
	writeVec3("Rot", Player::s_sheathedRotL, true);
	file << "\t}\n";

	file << "}\n";
}