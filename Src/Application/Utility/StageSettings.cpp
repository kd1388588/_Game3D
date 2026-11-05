#include "StageSettings.h"

// JSON の読み書き（nlohmann/json。tinygltf と一緒に Library/tinygltf に入っている）
#include "json.hpp"

namespace
{
	using json = nlohmann::json;

	// ---- JSON との変換 ----
	json ToJson(const Math::Vector3& v) { return json::array({ v.x, v.y, v.z }); }

	void Read(const json& j, const char* key, Math::Vector3& out)
	{
		if (!j.contains(key) || !j[key].is_array() || j[key].size() != 3) return;
		out = { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>() };
	}

	template <class T>
	void Read(const json& j, const char* key, T& out)
	{
		if (j.contains(key)) out = j[key].get<T>();
	}

	// ---- エディタの部品 ----
	// 項目の横に「?」を出し、マウスを乗せると説明が出る
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

	// 細かく調整できる数値（ドラッグで動かす。Alt を押しながらだと、もっと細かく動く）
	void Drag(const char* label, float& value, float speed, float min, float max, const char* format, const char* help)
	{
		ImGui::DragFloat(label, &value, speed, min, max, format, ImGuiSliderFlags_AlwaysClamp);
		Help(help);
	}
}

void StageSettings::ApplyFog() const
{
	KdAmbientController& ambient = KdShaderManager::Instance().WorkAmbientController();
	ambient.SetFogEnable(m_fog.distanceEnable, m_fog.heightEnable);
	ambient.SetDistanceFog(m_fog.distanceColor, m_fog.distanceDensity);
	ambient.SetheightFog(m_fog.heightColor, m_fog.heightTop, m_fog.heightBottom, m_fog.heightDistance);
}

// ============================================================
// 保存・読み込み
// ============================================================
bool StageSettings::Save(const std::string& path) const
{
	json j;
	j["Fog"] =
	{
		{ "DistanceEnable",		m_fog.distanceEnable },
		{ "DistanceColor",		ToJson(m_fog.distanceColor) },
		{ "DistanceDensity",	m_fog.distanceDensity },
		{ "HeightEnable",		m_fog.heightEnable },
		{ "HeightColor",		ToJson(m_fog.heightColor) },
		{ "HeightTop",			m_fog.heightTop },
		{ "HeightBottom",		m_fog.heightBottom },
		{ "HeightDistance",		m_fog.heightDistance },
	};
	const GroundMist& m = m_groundMist;
	j["GroundMist"] =
	{
		{ "Enable",				m.enable },
		{ "Count",				m.count },
		{ "Brightness",			m.brightness },
		{ "BrightnessRandom",	m.brightnessRandom },
		{ "Color",				ToJson(m.color) },
		{ "SizeMin",			m.sizeMin },
		{ "SizeMax",			m.sizeMax },
		{ "Flatten",			m.flatten },
		{ "BottomHeight",		m.bottomHeight },
		{ "HeightRandom",		m.heightRandom },
		{ "Range",				m.range },
		{ "NearFadeStart",		m.nearFadeStart },
		{ "NearFadeLength",		m.nearFadeLength },
		{ "DriftSpeed",			m.driftSpeed },
		{ "DriftAngle",			m.driftAngle },
		{ "SwaySpeed",			m.swaySpeed },
		{ "SwayRange",			m.swayRange },
	};

	const LowHpScreen& l = m_lowHp;
	j["LowHpScreen"] =
	{
		{ "Enable",			l.enable },
		{ "StartRate",		l.startRate },
		{ "Grayscale",		l.grayscale },
		{ "Darken",			l.darken },
		{ "Vignette",		l.vignette },
		{ "VignetteStart",	l.vignetteStart },
		{ "FollowRate",		l.followRate },
		{ "BlinkRate",		l.blinkRate },
		{ "BlinkSpeed",		l.blinkSpeed },
	};

	std::ofstream file(path);
	if (!file) return false;
	file << j.dump(4);
	return true;
}

bool StageSettings::Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file) return false;

	// 書き方がおかしいファイルでも止まらないように、例外を出さずに読む
	json j = json::parse(file, nullptr, false);
	if (j.is_discarded()) return false;

	if (j.contains("Fog"))
	{
		const json& f = j["Fog"];
		Read(f, "DistanceEnable",	m_fog.distanceEnable);
		Read(f, "DistanceColor",	m_fog.distanceColor);
		Read(f, "DistanceDensity",	m_fog.distanceDensity);
		Read(f, "HeightEnable",		m_fog.heightEnable);
		Read(f, "HeightColor",		m_fog.heightColor);
		Read(f, "HeightTop",		m_fog.heightTop);
		Read(f, "HeightBottom",		m_fog.heightBottom);
		Read(f, "HeightDistance",	m_fog.heightDistance);
	}
	if (j.contains("GroundMist"))
	{
		const json& g = j["GroundMist"];
		GroundMist& m = m_groundMist;
		Read(g, "Enable",			m.enable);
		Read(g, "Count",			m.count);
		Read(g, "Brightness",		m.brightness);
		Read(g, "BrightnessRandom",	m.brightnessRandom);
		Read(g, "Color",			m.color);
		Read(g, "SizeMin",			m.sizeMin);
		Read(g, "SizeMax",			m.sizeMax);
		Read(g, "Flatten",			m.flatten);
		Read(g, "BottomHeight",		m.bottomHeight);
		Read(g, "HeightRandom",		m.heightRandom);
		Read(g, "Range",			m.range);
		Read(g, "NearFadeStart",	m.nearFadeStart);
		Read(g, "NearFadeLength",	m.nearFadeLength);
		Read(g, "DriftSpeed",		m.driftSpeed);
		Read(g, "DriftAngle",		m.driftAngle);
		Read(g, "SwaySpeed",		m.swaySpeed);
		Read(g, "SwayRange",		m.swayRange);
	}
	if (j.contains("LowHpScreen"))
	{
		const json& h = j["LowHpScreen"];
		LowHpScreen& l = m_lowHp;
		Read(h, "Enable",			l.enable);
		Read(h, "StartRate",		l.startRate);
		Read(h, "Grayscale",		l.grayscale);
		Read(h, "Darken",			l.darken);
		Read(h, "Vignette",			l.vignette);
		Read(h, "VignetteStart",	l.vignetteStart);
		Read(h, "FollowRate",		l.followRate);
		Read(h, "BlinkRate",		l.blinkRate);
		Read(h, "BlinkSpeed",		l.blinkSpeed);
	}
	return true;
}

// ============================================================
// ステージエディタ
// ============================================================
void StageSettings::DrawEditor()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(420, 640), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Stage Editor"))
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
	if (ImGui::Button(U8("すべて初期値に戻す")))
	{
		ResetAll();
		m_message = U8("初期値に戻しました（保存するまでファイルは変わりません）");
	}
	if (!m_message.empty()) ImGui::TextWrapped("%s", m_message.c_str());

	ImGui::TextDisabled(U8("ドラッグで調整／Alt＋ドラッグで細かく／Shift＋ドラッグで大きく"));
	ImGui::TextDisabled(U8("Ctrl＋クリック（またはダブルクリック）で数値を直接入力"));
	ImGui::Separator();

	if (ImGui::BeginTabBar("StageEditorTabs"))
	{
		if (ImGui::BeginTabItem(U8("霧")))
		{
			DrawFogEditor();
			DrawGroundMistEditor();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem(U8("画面")))
		{
			DrawLowHpEditor();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

void StageSettings::DrawFogEditor()
{
	Fog& f = m_fog;

	if (ImGui::CollapsingHeader(U8("距離の霧（遠くを夜の空気の色に溶かす）"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("DistanceFog");
		ImGui::Checkbox(U8("使う"), &f.distanceEnable);
		ImGui::ColorEdit3(U8("色"), &f.distanceColor.x, ImGuiColorEditFlags_Float);
		Drag(U8("濃さ"), f.distanceDensity, 0.0001f, 0.0f, 0.1f, "%.4f",
			U8("大きいほど、近くから霧の色になる。20m 先に残る元の色 ＝ 1 / exp(20 × 濃さ)"));
		ImGui::Text(U8("　20m 先：%.0f%%　50m 先：%.0f%% だけ元の色が残る"),
			100.0f / std::exp(20.0f * f.distanceDensity), 100.0f / std::exp(50.0f * f.distanceDensity));
		if (ImGui::Button(U8("距離の霧を初期値に戻す")))
		{
			Fog def;
			f.distanceEnable = def.distanceEnable;
			f.distanceColor = def.distanceColor;
			f.distanceDensity = def.distanceDensity;
		}
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(U8("高さの霧（地面の近くを、遠くほど霧の色にする）"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushID("HeightFog");
		ImGui::Checkbox(U8("使う"), &f.heightEnable);
		ImGui::ColorEdit3(U8("色"), &f.heightColor.x, ImGuiColorEditFlags_Float);
		Drag(U8("上の高さ"), f.heightTop, 0.01f, -10.0f, 20.0f, "%.2f m", U8("この高さより下に霧がかかる"));
		Drag(U8("下の高さ"), f.heightBottom, 0.01f, -10.0f, 20.0f, "%.2f m", U8("この高さで一番濃くなる（上の高さより低くする）"));
		Drag(U8("一番濃くなる距離"), f.heightDistance, 0.1f, 1.0f, 300.0f, "%.1f m", U8("カメラからこの距離で一番濃くなる。小さいほど近くから霧が見える"));
		if (ImGui::Button(U8("高さの霧を初期値に戻す")))
		{
			Fog def;
			f.heightEnable = def.heightEnable;
			f.heightColor = def.heightColor;
			f.heightTop = def.heightTop;
			f.heightBottom = def.heightBottom;
			f.heightDistance = def.heightDistance;
		}
		ImGui::PopID();
	}
}

void StageSettings::DrawGroundMistEditor()
{
	GroundMist& m = m_groundMist;

	if (!ImGui::CollapsingHeader(U8("地面の霧（床の近くに漂う霧のかたまり）"), ImGuiTreeNodeFlags_DefaultOpen)) return;

	ImGui::PushID("GroundMist");
	ImGui::Checkbox(U8("使う"), &m.enable);

	ImGui::SeparatorText(U8("濃さ・色"));
	Drag(U8("濃さ"), m.brightness, 0.001f, 0.0f, 1.0f, "%.3f", U8("かたまり1つの濃さ。加算で重ねるので、大きいほど白く明るくなる"));
	Drag(U8("濃さのばらつき"), m.brightnessRandom, 0.001f, 0.0f, 1.0f, "%.3f", U8("かたまりごとに、濃さをこれだけばらつかせる"));
	ImGui::ColorEdit3(U8("色"), &m.color.x, ImGuiColorEditFlags_Float);
	ImGui::SliderInt(U8("かたまりの数"), &m.count, 0, 300);
	Help(U8("多いほど、すき間なく広がる（多すぎると重くなる）"));

	ImGui::SeparatorText(U8("形・高さ"));
	Drag(U8("幅（最小）"), m.sizeMin, 0.01f, 0.1f, 50.0f, "%.2f m", U8("かたまりの幅は、最小～最大の間でばらつく"));
	Drag(U8("幅（最大）"), m.sizeMax, 0.01f, 0.1f, 50.0f, "%.2f m", U8("かたまりの幅は、最小～最大の間でばらつく"));
	if (m.sizeMax < m.sizeMin) m.sizeMax = m.sizeMin;
	Drag(U8("平べったさ"), m.flatten, 0.001f, 0.02f, 2.0f, "%.3f", U8("高さ ÷ 幅。小さいほど平べったく、床をはうように見える"));
	Drag(U8("下の端の高さ"), m.bottomHeight, 0.005f, -3.0f, 5.0f, "%.3f m", U8("床から、かたまりの下の端までの高さ。マイナスにすると床にしずむ（床との境目が見えにくくなる）"));
	Drag(U8("高さのばらつき"), m.heightRandom, 0.005f, 0.0f, 5.0f, "%.3f m", U8("かたまりごとに、高さをこれだけばらつかせる"));

	ImGui::SeparatorText(U8("出す範囲"));
	Drag(U8("範囲"), m.range, 0.1f, 5.0f, 150.0f, "%.1f m", U8("カメラのまわり、この距離までに出す。外に出たかたまりは、反対側に出し直す"));
	Drag(U8("近くで薄くし始める距離"), m.nearFadeStart, 0.01f, 0.0f, 30.0f, "%.2f m", U8("カメラにこれより近づいたかたまりは薄くする（目の前が白くならないように）"));
	Drag(U8("薄くする長さ"), m.nearFadeLength, 0.01f, 0.01f, 30.0f, "%.2f m", U8("薄くし始めてから、これだけ近づくと完全に消える"));

	ImGui::SeparatorText(U8("動き"));
	Drag(U8("流れる速さ"), m.driftSpeed, 0.005f, 0.0f, 10.0f, "%.3f m/s", U8("霧が流れていく速さ（1秒に何 m 進むか）"));
	Drag(U8("流れる向き"), m.driftAngle, 0.5f, -180.0f, 180.0f, "%.1f deg", U8("0 で奥（+Z）へ、90 で右（+X）へ流れる"));
	Drag(U8("ゆらぎの速さ"), m.swaySpeed, 0.005f, 0.0f, 5.0f, "%.3f", U8("かたまりの濃さが、ゆっくり濃くなったり薄くなったりする速さ"));
	Drag(U8("ゆらぎの強さ"), m.swayRange, 0.005f, 0.0f, 1.0f, "%.3f", U8("0 でゆらがない。1 で 0 ～ 2 倍の間でゆらぐ"));

	if (ImGui::Button(U8("地面の霧を初期値に戻す"))) m = GroundMist();
	ImGui::PopID();
}

void StageSettings::DrawLowHpEditor()
{
	LowHpScreen& l = m_lowHp;

	if (!ImGui::CollapsingHeader(U8("HP が少ない時（白黒・暗く・周辺減光）"), ImGuiTreeNodeFlags_DefaultOpen)) return;

	ImGui::PushID("LowHp");
	ImGui::Checkbox(U8("使う"), &l.enable);
	Drag(U8("始まる HP"), l.startRate, 0.005f, 0.0f, 1.0f, "%.3f", U8("HP の割合（0～1）がこれより少なくなると始まり、HP 0 で一番強くなる。0.3 なら HP 30%"));

	ImGui::SeparatorText(U8("HP 0 の時の強さ"));
	Drag(U8("白黒"), l.grayscale, 0.005f, 0.0f, 1.0f, "%.3f", U8("1 で完全な白黒。0.7 くらいにすると、少し色が残る"));
	Drag(U8("暗さ"), l.darken, 0.005f, 0.0f, 1.0f, "%.3f", U8("画面全体を暗くする強さ"));
	Drag(U8("周辺減光"), l.vignette, 0.005f, 0.0f, 1.0f, "%.3f", U8("画面のふちを暗くする強さ。1 でふちが真っ黒"));
	Drag(U8("周辺減光の始まる位置"), l.vignetteStart, 0.005f, 0.0f, 1.5f, "%.3f", U8("画面の真ん中からの距離。0 で真ん中から、1 で上下左右のふちから暗くし始める"));

	ImGui::SeparatorText(U8("動き"));
	Drag(U8("変わる速さ"), l.followRate, 0.001f, 0.001f, 1.0f, "%.3f", U8("HP が変わった時に、画面がその強さに近づく速さ（1 ですぐ変わる。0.05 で約1秒）"));
	Drag(U8("HP が点滅し始める HP"), l.blinkRate, 0.005f, 0.0f, 1.0f, "%.3f", U8("HP ゲージが赤く点滅し始める HP の割合"));
	Drag(U8("点滅の速さ"), l.blinkSpeed, 0.001f, 0.0f, 1.0f, "%.3f", U8("大きいほど速く点滅する"));

	if (ImGui::Button(U8("HP が少ない時の画面を初期値に戻す"))) l = LowHpScreen();
	ImGui::PopID();
}
