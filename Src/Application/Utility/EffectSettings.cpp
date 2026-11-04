#include "EffectSettings.h"
#include "../GameObject/Effect/EffectPlayer.h"

// JSON の読み書き（nlohmann/json。tinygltf と一緒に Library/tinygltf に入っている）
#include "json.hpp"

namespace
{
	using json = nlohmann::json;

	// プレイヤーのモードの色（剣の軌跡・リムライトと同じ系統。掛け算なので、少し明るめにしてある）
	const Math::Color kOperatorTint		= { 0.55f, 0.8f, 1.0f, 1.0f };		// 青
	const Math::Color kPunisherTint		= { 1.0f, 0.6f, 0.3f, 1.0f };		// オレンジ
	const Math::Color kEnemyClawColor	= { 1.0f, 0.3f, 0.25f, 1.0f };		// 敵の爪（赤）

	json ToJson(const Math::Color& c) { return json::array({ c.x, c.y, c.z, c.w }); }

	void Read(const json& j, const char* key, Math::Color& out)
	{
		if (!j.contains(key) || !j[key].is_array() || j[key].size() != 4) return;
		out = { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>(), j[key][3].get<float>() };
	}

	template <class T>
	void Read(const json& j, const char* key, T& out)
	{
		if (j.contains(key)) out = j[key].get<T>();
	}

	void Help(const char* text)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
			ImGui::TextUnformatted(text);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	bool StartsWith(const std::string& s, const char* prefix)
	{
		return s.rfind(prefix, 0) == 0;
	}

	constexpr float kFrameSec = 1.0f / 60.0f;
}

// ============================================================
// 設定
// ============================================================
EffectSettings::Entry EffectSettings::MakeDefault(const std::string& fileName)
{
	Entry e;
	e.operatorColor = kOperatorTint;
	e.punisherColor = kPunisherTint;

	// プレイヤーの斬撃（_Sword…）とモード切り替えは、モードの色にする
	if (StartsWith(fileName, "_Sword") || fileName == EffectName::ModeChange)
	{
		e.useModeColor = true;
	}
	// モード切り替えは、前からコードで付けていた色（モードの色を白に半分混ぜたもの）
	if (fileName == EffectName::ModeChange)
	{
		e.operatorColor = { 0.5f, 0.75f, 1.0f, 1.0f };
		e.punisherColor = { 1.0f, 0.65f, 0.5f, 1.0f };
	}
	// 敵の爪は赤（前に Enemy.cpp の kHitEffectColor で付けていた色）
	if (fileName == EffectName::HitPlayer)
	{
		e.color = kEnemyClawColor;
	}
	return e;
}

const EffectSettings::Entry& EffectSettings::Get(const std::string& fileName)
{
	auto it = m_entries.find(fileName);
	if (it != m_entries.end()) return it->second;
	return m_entries.emplace(fileName, MakeDefault(fileName)).first->second;
}

Math::Color EffectSettings::GetColor(const std::string& fileName)
{
	const Entry& e = Get(fileName);
	if (!e.useModeColor) return e.color;

	bool punisher = m_isPlayerPunisher;
	if (m_previewMode == 1) punisher = false;
	if (m_previewMode == 2) punisher = true;
	return punisher ? e.punisherColor : e.operatorColor;
}

void EffectSettings::OnGameStart()
{
	Load();
	if (m_syncAutoStart)
	{
		KdEffekseerManager::GetInstance().PreloadAll();
		KdEffekseerManager::GetInstance().StartServer(static_cast<uint16_t>(m_syncPort));
	}
}

// ============================================================
// 保存・読み込み
// ============================================================
bool EffectSettings::Save(const std::string& path) const
{
	json j;
	j["SyncAutoStart"] = m_syncAutoStart;
	j["SyncPort"] = m_syncPort;
	json effects = json::object();
	for (const auto& [name, e] : m_entries)
	{
		effects[name] =
		{
			{ "Scale",			e.scale },
			{ "Speed",			e.speed },
			{ "HeightOffset",	e.heightOffset },
			{ "UseModeColor",	e.useModeColor },
			{ "Color",			ToJson(e.color) },
			{ "OperatorColor",	ToJson(e.operatorColor) },
			{ "PunisherColor",	ToJson(e.punisherColor) },
		};
	}
	j["Effects"] = effects;

	std::ofstream file(path);
	if (!file) return false;
	file << j.dump(4);
	return true;
}

bool EffectSettings::Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file) return false;

	json j = json::parse(file, nullptr, false);
	if (j.is_discarded()) return false;

	Read(j, "SyncAutoStart", m_syncAutoStart);
	Read(j, "SyncPort", m_syncPort);
	if (j.contains("Effects") && j["Effects"].is_object())
	{
		for (const auto& [name, v] : j["Effects"].items())
		{
			Entry e = MakeDefault(name);
			Read(v, "Scale",			e.scale);
			Read(v, "Speed",			e.speed);
			Read(v, "HeightOffset",		e.heightOffset);
			Read(v, "UseModeColor",		e.useModeColor);
			Read(v, "Color",			e.color);
			Read(v, "OperatorColor",	e.operatorColor);
			Read(v, "PunisherColor",	e.punisherColor);
			m_entries[name] = e;
		}
	}
	return true;
}

// ============================================================
// エフェクトエディタ
// ============================================================
void EffectSettings::RefreshFileList()
{
	m_fileList.clear();
	std::error_code ec;
	for (const auto& file : std::filesystem::directory_iterator(EffekseerPath, ec))
	{
		if (file.is_regular_file() && file.path().extension() == ".efkefc")
		{
			m_fileList.push_back(file.path().filename().string());
		}
	}
	std::sort(m_fileList.begin(), m_fileList.end());
	if (m_selected.empty() && !m_fileList.empty()) m_selected = m_fileList.front();
}

void EffectSettings::PlayPreview(const std::string& fileName)
{
	// カメラの前（水平に previewDistance m 先、カメラより少し下）に、カメラの方を向けて出す
	Math::Matrix camMat = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	Math::Vector3 forward = camMat.Backward();
	forward.y = 0.0f;
	if (forward.LengthSquared() > 0.0f) forward.Normalize();
	Math::Vector3 pos = camMat.Translation() + forward * m_previewDistance;
	pos.y -= 0.8f;
	EffectPlayer::PlayFacingCamera(fileName, pos, m_previewScale);
}

void EffectSettings::DrawEditor()
{
	if (m_fileList.empty()) RefreshFileList();

	// 続けて試し撃ち（デバッグ表示中だけ動く）
	if (m_previewRepeat && !m_selected.empty())
	{
		m_previewTimer += kFrameSec;
		if (m_previewTimer >= m_previewInterval)
		{
			m_previewTimer = 0.0f;
			PlayPreview(m_selected);
		}
	}

	ImGui::SetNextWindowPos(ImVec2(440, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(460, 680), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Effect Editor"))
	{
		ImGui::End();
		return;
	}

	if (ImGui::Button(U8("保存")))
	{
		m_message = Save() ? U8("保存しました：") + kDefaultPath : U8("保存できませんでした：") + kDefaultPath;
	}
	ImGui::SameLine();
	if (ImGui::Button(U8("読み込み")))
	{
		m_message = Load() ? U8("読み込みました：") + kDefaultPath : U8("読み込めませんでした（ファイルが無い・書き方が違う）");
	}
	ImGui::SameLine();
	if (ImGui::Button(U8("一覧を更新")))
	{
		RefreshFileList();
	}
	if (!m_message.empty()) ImGui::TextWrapped("%s", m_message.c_str());
	ImGui::TextDisabled(U8("ドラッグで調整／Alt＋ドラッグで細かく／Ctrl＋クリックで数値を入力"));
	ImGui::Separator();

	DrawSyncEditor();

	// ---- 試し撃ち ----
	if (ImGui::CollapsingHeader(U8("試し撃ち（カメラの前にエフェクトを出す）"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text(U8("今のプレイヤーのモード：%s"), m_isPlayerPunisher ? U8("パニッシャー") : U8("オペレーター"));
		ImGui::RadioButton(U8("今のモードの色"), &m_previewMode, 0);
		ImGui::SameLine();
		ImGui::RadioButton(U8("オペレーター"), &m_previewMode, 1);
		ImGui::SameLine();
		ImGui::RadioButton(U8("パニッシャー"), &m_previewMode, 2);
		Help(U8("モードの色を使うエフェクトを、どちらの色で出すか。ゲームの中で出るエフェクトにも効くので、確認が終わったら「今のモードの色」に戻す"));
		ImGui::DragFloat(U8("試し撃ちの大きさ"), &m_previewScale, 0.005f, 0.01f, 5.0f, "%.3f");
		ImGui::DragFloat(U8("カメラからの距離"), &m_previewDistance, 0.05f, 0.5f, 30.0f, "%.2f m");
		if (ImGui::Button(U8("選んだエフェクトを出す")) && !m_selected.empty()) PlayPreview(m_selected);
		ImGui::SameLine();
		ImGui::Checkbox(U8("続けて出す"), &m_previewRepeat);
		ImGui::DragFloat(U8("続けて出す間隔"), &m_previewInterval, 0.01f, 0.1f, 10.0f, "%.2f s");
	}

	// ---- エフェクトの一覧と設定 ----
	if (ImGui::CollapsingHeader(U8("エフェクトごとの設定"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::BeginChild("EffectList", ImVec2(170, 300), true);
		for (const std::string& name : m_fileList)
		{
			if (ImGui::Selectable(name.c_str(), m_selected == name)) m_selected = name;
		}
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("EffectDetail", ImVec2(0, 300), true);
		if (!m_selected.empty())
		{
			Get(m_selected);	// まだ無ければ初期値を作る
			DrawEntryEditor(m_selected, m_entries[m_selected]);
		}
		ImGui::EndChild();
	}

	ImGui::End();
}

void EffectSettings::DrawSyncEditor()
{
	if (!ImGui::CollapsingHeader(U8("Effekseer と同期（エディタの編集を、ゲームにすぐ反映）"))) return;

	KdEffekseerManager& manager = KdEffekseerManager::GetInstance();
	bool running = manager.IsServerRunning();

	ImGui::TextWrapped(U8("1. ここで「同期を開始」を押す\n2. Effekseer エディタで同じ名前のエフェクト（例：_Sword1-1.efkefc）を開く\n3. エディタの「ネットワーク」で、ホスト 127.0.0.1・ポート下の番号に接続する\n4. エディタで編集すると、ゲームの中のエフェクトがすぐ変わる\n5. よくなったら、エディタで .efkefc を保存する（ゲームを終えると、同期した内容は消える）"));
	ImGui::Text(U8("状態：%s"), running ? U8("同期中（エディタから接続できます）") : U8("止まっています"));

	if (running) ImGui::BeginDisabled();
	ImGui::InputInt(U8("ポート"), &m_syncPort);
	m_syncPort = std::clamp(m_syncPort, 1024, 65535);
	if (running) ImGui::EndDisabled();

	if (!running)
	{
		if (ImGui::Button(U8("同期を開始")))
		{
			int loaded = manager.PreloadAll();
			bool ok = manager.StartServer(static_cast<uint16_t>(m_syncPort));
			char buf[160];
			sprintf_s(buf, U8("同期を開始%s（エフェクトを %d 個読み込みました）"), ok ? U8("しました") : U8("できませんでした"), loaded);
			m_message = buf;
		}
	}
	else
	{
		if (ImGui::Button(U8("同期を止める")))
		{
			manager.StopServer();
			m_message = U8("同期を止めました");
		}
	}
	ImGui::SameLine();
	ImGui::Checkbox(U8("ゲームを始めた時に自動で開始"), &m_syncAutoStart);
	Help(U8("保存すると、次からゲームを始めた時に同期を始める。初めて同期する時は、Windows のファイアウォールの確認が出ることがある"));
}

void EffectSettings::DrawEntryEditor(const std::string& fileName, Entry& e)
{
	ImGui::PushID(fileName.c_str());
	ImGui::Text("%s", fileName.c_str());
	ImGui::Separator();

	ImGui::DragFloat(U8("大きさ"), &e.scale, 0.001f, 0.01f, 10.0f, "%.3f");
	Help(U8("ゲームの中で出す時の大きさに、さらに掛ける倍率（1.0 でそのまま）"));
	ImGui::DragFloat(U8("再生速度"), &e.speed, 0.001f, 0.01f, 10.0f, "%.3f");
	Help(U8("1.0 でそのまま。小さいほどゆっくり"));
	ImGui::DragFloat(U8("高さのずれ"), &e.heightOffset, 0.001f, -5.0f, 5.0f, "%.3f m");
	Help(U8("出す位置を上下にずらす"));

	ImGui::SeparatorText(U8("色（エフェクトの元の色に掛け算される）"));
	ImGui::Checkbox(U8("プレイヤーのモードで色を変える"), &e.useModeColor);
	if (e.useModeColor)
	{
		ImGui::ColorEdit4(U8("オペレーター"), &e.operatorColor.x, ImGuiColorEditFlags_Float);
		ImGui::ColorEdit4(U8("パニッシャー"), &e.punisherColor.x, ImGuiColorEditFlags_Float);
	}
	else
	{
		ImGui::ColorEdit4(U8("色"), &e.color.x, ImGuiColorEditFlags_Float);
	}
	ImGui::TextWrapped(U8("※掛け算なので、白（1.0）より明るくはできない。元のエフェクトに色が付いていると、暗く濁ることがある（その時は Effekseer で元の色を白っぽくする）"));

	if (ImGui::Button(U8("このエフェクトを初期値に戻す"))) e = MakeDefault(fileName);
	ImGui::SameLine();
	if (ImGui::Button(U8("出してみる"))) PlayPreview(fileName);

	ImGui::PopID();
}
