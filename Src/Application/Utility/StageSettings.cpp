#include "StageSettings.h"

// JSON の読み書き（nlohmann/json。tinygltf と一緒に Library/tinygltf に入っている）
#include "json.hpp"

// 「Enemy」タブの「今いる敵に反映」「ボスをブレイクさせる」で、今いる敵を探すため
#include "../Scene/SceneManager.h"
#include "../GameObject/Character/Enemy/Enemy.h"

// 「Player」タブ：プレイヤーの調整用の値（Player の static 変数・PlayerParamManager）
#include "../GameObject/Character/Player/Player.h"
#include "../GameObject/Character/Player/PlayerParamManager.h"

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

	// 整数の数値（HP・ダメージ・フレーム数など）
	void DragI(const char* label, int& value, float speed, int min, int max, const char* help)
	{
		ImGui::DragInt(label, &value, speed, min, max, "%d", ImGuiSliderFlags_AlwaysClamp);
		Help(help);
	}

	// ---- 戦闘の数値の項目の一覧（保存と読み込みで、同じ一覧を使う） ----
	// 【なぜ一覧にするのか】保存と読み込みで別々に書くと、片方だけ書き忘れることがあるため
	//   f(JSON の名前, 値) を項目の数だけ呼ぶ。保存では「値を書く」、読み込みでは「値を読む」関数を渡す
	template <class T, class F>
	void VisitPlayerTuning(T& t, F&& f)
	{
		f("MaxHp", t.maxHp);						f("AttackDamage", t.attackDamage);
		f("CriticalAttackDamage", t.criticalAttackDamage);	f("CritRate", t.critRate);
		f("OperatorDamageRate", t.operatorDamageRate);	f("PunisherDamageRate", t.punisherDamageRate);
		f("AbilityDamageRate", t.abilityDamageRate);	f("MultiHitInterval", t.multiHitInterval);
		f("AbilityStunFrame", t.abilityStunFrame);
		f("OperatorAttackSpeedRate", t.operatorAttackSpeedRate);	f("PunisherAttackSpeedRate", t.punisherAttackSpeedRate);
		f("PunisherMoveRate", t.punisherMoveRate);		f("PunisherTakenDamageRate", t.punisherTakenDamageRate);
		f("DamageInvincibleFrame", t.damageInvincibleFrame);	f("HeavyDamage", t.heavyDamage);
		f("EvadeDamageRate", t.evadeDamageRate);		f("GuardDamageRate", t.guardDamageRate);
		f("AtbHitGain", t.atbHitGain);				f("OperatorAtbHitRate", t.operatorAtbHitRate);
		f("PunisherAtbHitRate", t.punisherAtbHitRate);	f("AtbCriticalRate", t.atbCriticalRate);
		f("AtbEvadeGain", t.atbEvadeGain);
		f("GuardDurabilityMax", t.guardDurabilityMax);	f("GuardCostPerDamage", t.guardCostPerDamage);
		f("GuardCriticalCostRate", t.guardCriticalCostRate);	f("GuardRegenDelay", t.guardRegenDelay);
		f("GuardRegenSpeed", t.guardRegenSpeed);		f("GuardBreakFrame", t.guardBreakFrame);
		f("LockOnRange", t.lockOnRange);
	}

	template <class T, class F>
	void VisitEnemyType(T& e, F&& f)
	{
		f("Hp", e.hp);	f("Scale", e.scale);	f("SearchRange", e.searchRange);
		f("AttackRange", e.attackRange);	f("JumpAttackCT", e.jumpAttackCT);
	}

	template <class T, class F>
	void VisitEnemyTuning(T& t, F&& f)
	{
		f("AttackDamage", t.attackDamage);			f("CriticalAttackDamage", t.criticalAttackDamage);
		f("SmashDamage", t.smashDamage);			f("AttackRecoverFrame", t.attackRecoverFrame);
		f("BossAttackRecoverFrame", t.bossAttackRecoverFrame);	f("AttackRecoverRandom", t.attackRecoverRandom);
		f("JumpAttackRecoverFrame", t.jumpAttackRecoverFrame);	f("JumpAttackChance", t.jumpAttackChance);
		f("JumpAttackMinDist", t.jumpAttackMinDist);	f("JumpAttackMaxDist", t.jumpAttackMaxDist);
		f("ChaseSpeed", t.chaseSpeed);
		f("KnockbackSpeed", t.knockbackSpeed);		f("KnockbackCriticalRate", t.knockbackCriticalRate);
		f("KnockbackDecay", t.knockbackDecay);		f("KnockbackArmorRate", t.knockbackArmorRate);
		f("KnockbackStunRate", t.knockbackStunRate);	f("KnockbackMidBossRate", t.knockbackMidBossRate);
		f("KnockbackBossRate", t.knockbackBossRate);
	}

	// 一覧を JSON に書く・JSON から読む
	template <class T, class V>
	json WriteList(const T& t, V visit)
	{
		json j = json::object();
		visit(t, [&](const char* key, const auto& value) { j[key] = value; });
		return j;
	}
	template <class T, class V>
	void ReadList(const json& j, T& t, V visit)
	{
		visit(t, [&](const char* key, auto& value) { Read(j, key, value); });
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
		{ "YellowRate",		l.yellowRate },
		{ "BlinkRate",		l.blinkRate },
		{ "BlinkSpeed",		l.blinkSpeed },
		{ "FastBlinkSpeed",	l.fastBlinkSpeed },
		{ "DeathFadeSec",	l.deathFadeSec },
	};

	// プレイヤーの調整用の値（回避の速さ・剣の持ち方は、PlayerParamManager が別のファイルに保存する）
	j["Player"] =
	{
		{ "RunSpeedRate",		Player::s_runSpeedRate },
		{ "UseRootMotion",		Player::s_useRootMotion },
		{ "BladeShapeHitbox",	Player::s_bladeShapeHitbox },
		{ "ThrowAngleOffset",	Player::s_throwAngleOffset },
	};

	const Battle& b = m_battle;
	j["Battle"] =
	{
		{ "BreakEnable",		b.breakEnable },
		{ "NormalBreak",		b.normalBreak },
		{ "MidBossBreak",		b.midBossBreak },
		{ "BreakMax",			b.breakMax },
		{ "HitGain",			b.hitGain },
		{ "CriticalRate",		b.criticalRate },
		{ "PunisherRate",		b.punisherRate },
		{ "AbilityRate",		b.abilityRate },
		{ "BonusGain",			b.bonusGain },
		{ "DecayDelaySec",		b.decayDelaySec },
		{ "DecayPerSec",		b.decayPerSec },
		{ "BreakSec",			b.breakSec },
		{ "BreakDamageRate",	b.breakDamageRate },
	};

	// プレイヤー・敵の戦闘の数値
	auto visitPlayer = [](auto& t, auto&& f) { VisitPlayerTuning(t, f); };
	auto visitType = [](auto& t, auto&& f) { VisitEnemyType(t, f); };
	auto visitEnemy = [](auto& t, auto&& f) { VisitEnemyTuning(t, f); };
	j["PlayerTuning"] = WriteList(m_player, visitPlayer);
	j["EnemyTuning"] = WriteList(m_enemy, visitEnemy);
	j["EnemyTuning"]["Normal"] = WriteList(m_enemy.normal, visitType);
	j["EnemyTuning"]["MidBoss"] = WriteList(m_enemy.midBoss, visitType);
	j["EnemyTuning"]["Boss"] = WriteList(m_enemy.boss, visitType);

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
	// （parse の 3 つ目の引数を false にすると、失敗した時に is_discarded() が true になる）
	// 【なぜ Read で 1 つずつ読むのか】ファイルに無い項目は、今の値（初期値）のまま残すため
	//   新しい項目を足しても、古い JSON ファイルがそのまま使える
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
		Read(h, "YellowRate",		l.yellowRate);
		Read(h, "BlinkRate",		l.blinkRate);
		Read(h, "BlinkSpeed",		l.blinkSpeed);
		Read(h, "FastBlinkSpeed",	l.fastBlinkSpeed);
		Read(h, "DeathFadeSec",		l.deathFadeSec);
	}
	if (j.contains("Player"))
	{
		const json& p = j["Player"];
		Read(p, "RunSpeedRate",		Player::s_runSpeedRate);
		Read(p, "UseRootMotion",	Player::s_useRootMotion);
		Read(p, "BladeShapeHitbox",	Player::s_bladeShapeHitbox);
		Read(p, "ThrowAngleOffset",	Player::s_throwAngleOffset);
	}
	auto visitPlayer = [](auto& t, auto&& f) { VisitPlayerTuning(t, f); };
	auto visitType = [](auto& t, auto&& f) { VisitEnemyType(t, f); };
	auto visitEnemy = [](auto& t, auto&& f) { VisitEnemyTuning(t, f); };
	if (j.contains("PlayerTuning")) ReadList(j["PlayerTuning"], m_player, visitPlayer);
	if (j.contains("EnemyTuning"))
	{
		const json& e = j["EnemyTuning"];
		ReadList(e, m_enemy, visitEnemy);
		if (e.contains("Normal"))	ReadList(e["Normal"], m_enemy.normal, visitType);
		if (e.contains("MidBoss"))	ReadList(e["MidBoss"], m_enemy.midBoss, visitType);
		if (e.contains("Boss"))		ReadList(e["Boss"], m_enemy.boss, visitType);
	}
	if (j.contains("Battle"))
	{
		const json& t = j["Battle"];
		Battle& b = m_battle;
		Read(t, "BreakEnable",		b.breakEnable);
		Read(t, "NormalBreak",		b.normalBreak);
		Read(t, "MidBossBreak",		b.midBossBreak);
		Read(t, "BreakMax",			b.breakMax);
		Read(t, "HitGain",			b.hitGain);
		Read(t, "CriticalRate",		b.criticalRate);
		Read(t, "PunisherRate",		b.punisherRate);
		Read(t, "AbilityRate",		b.abilityRate);
		Read(t, "BonusGain",		b.bonusGain);
		Read(t, "DecayDelaySec",	b.decayDelaySec);
		Read(t, "DecayPerSec",		b.decayPerSec);
		Read(t, "BreakSec",			b.breakSec);
		Read(t, "BreakDamageRate",	b.breakDamageRate);
	}
	return true;
}

// ============================================================
// オブジェクトエディタ（Player / Enemy / Stage / UI のタブ）
// 【なぜ 1 つのウィンドウにまとめたか】前は「Player Parameter Editor」「Stage Editor」に分かれていて、
//   敵の HP が Stage Editor にあるなど、どこに何があるか分かりにくかったため
//   ※エフェクトは大きく、保存の仕方も違うので、Effect Editor のまま別のウィンドウにしている
// ============================================================
void StageSettings::DrawEditor()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(420, 640), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Object Editor"))
	{
		ImGui::End();
		return;
	}

	// 保存・読み込みは、全部のタブをまとめて行う
	//   StageSettings.json：Player（走る速さなど）・Enemy・Stage・UI
	//   PlayerParams.txt  ：回避の速さ（PlayerParamManager）
	//   WeaponParams.json ：剣の持ち方（PlayerParamManager）
	if (ImGui::Button(U8("保存")))
	{
		bool isOk = Save();
		PlayerParamManager::Instance().Save(kPlayerParamPath);
		PlayerParamManager::Instance().SaveWeaponParams(kWeaponParamPath);
		m_message = isOk ? U8("保存しました（Asset/Data/ の StageSettings.json・PlayerParams.txt・WeaponParams.json）")
						 : U8("保存できませんでした：") + kDefaultPath;
	}
	ImGui::SameLine();
	if (ImGui::Button(U8("読み込み")))
	{
		bool isOk = Load();
		PlayerParamManager::Instance().Load(kPlayerParamPath);
		PlayerParamManager::Instance().LoadWeaponParams(kWeaponParamPath);
		m_message = isOk ? U8("読み込みました") : U8("読み込めませんでした（ファイルが無い・書き方が違う）");
	}
	ImGui::SameLine();
	if (ImGui::Button(U8("Enemy・Stage・UI を初期値に戻す")))
	{
		ResetAll();
		m_message = U8("初期値に戻しました（Player タブは戻さない。保存するまでファイルは変わりません）");
	}
	if (!m_message.empty()) ImGui::TextWrapped("%s", m_message.c_str());

	ImGui::TextDisabled(U8("ドラッグで調整／Alt＋ドラッグで細かく／Shift＋ドラッグで大きく"));
	ImGui::TextDisabled(U8("Ctrl＋クリック（またはダブルクリック）で数値を直接入力"));
	ImGui::Separator();

	if (ImGui::BeginTabBar("ObjectEditorTabs"))
	{
		if (ImGui::BeginTabItem("Player"))
		{
			DrawPlayerEditor();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Enemy"))
		{
			DrawBattleEditor();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Stage"))
		{
			DrawFogEditor();
			DrawGroundMistEditor();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("UI"))
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

	ImGui::SeparatorText(U8("HP ゲージの色"));
	Drag(U8("黄色になる HP"), l.yellowRate, 0.005f, 0.0f, 1.0f, "%.3f", U8("HP の割合がこれより少なくなると、HP ゲージが黄色になる"));
	Drag(U8("赤く点滅する HP"), l.blinkRate, 0.005f, 0.0f, 1.0f, "%.3f", U8("HP の割合がこれより少なくなると、HP ゲージが赤くゆっくり点滅する"));
	Drag(U8("ゆっくりの点滅の速さ"), l.blinkSpeed, 0.001f, 0.0f, 1.0f, "%.3f", U8("大きいほど速く点滅する"));
	Drag(U8("速い点滅の速さ"), l.fastBlinkSpeed, 0.001f, 0.0f, 1.0f, "%.3f", U8("「始まる HP」（画面が暗くなり始める HP）より少ない時の点滅の速さ"));
	Drag(U8("倒れて真っ黒になるまで"), l.deathFadeSec, 0.01f, 0.1f, 10.0f, "%.2f s", U8("HP 0 で倒れてから、画面が真っ黒になるまでの時間。真っ黒になったらゲームオーバー画面へ移る"));

	if (ImGui::Button(U8("HP が少ない時の画面を初期値に戻す"))) l = LowHpScreen();
	ImGui::PopID();
}

void StageSettings::DrawBattleEditor()
{
	Battle& b = m_battle;
	ImGui::PushID("Battle");

	DrawLiveEnemyEditor();

	StageSettings::EnemyTuning& t = m_enemy;
	if (ImGui::CollapsingHeader(U8("種類ごとの数値（HP・大きさ・距離）"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto drawType = [](const char* name, StageSettings::EnemyType& e)
		{
			ImGui::SeparatorText(name);
			ImGui::PushID(name);
			DragI(U8("最大 HP"), e.hp, 1.0f, 1, 99999, U8("敵を作った時の HP"));
			Drag(U8("体の大きさ"), e.scale, 0.01f, 0.2f, 10.0f, "%.2f", U8("モデル・当たり判定・壁に当たる体の大きさ"));
			Drag(U8("気づく距離"), e.searchRange, 0.1f, 0.0f, 100.0f, "%.1f m", U8("プレイヤーがこれより近づくと、追いかけてくる"));
			Drag(U8("攻撃を始める距離"), e.attackRange, 0.05f, 0.0f, 20.0f, "%.2f m", U8("プレイヤーがこれより近いと、殴ってくる"));
			DragI(U8("飛びかかりの CT"), e.jumpAttackCT, 1.0f, 0, 60 * 120, U8("飛びかかり攻撃を使った後、次に使えるまでのフレーム数（60 で 1 秒）"));
			ImGui::PopID();
		};
		drawType(U8("ザコ"), t.normal);
		drawType(U8("中ボス"), t.midBoss);
		drawType(U8("ボス"), t.boss);

		if (ImGui::Button(U8("今いる敵に反映")))
		{
			// HP の残りの割合はそのままで、最大 HP・大きさ・距離を今の値に直す
			for (auto& obj : SceneManager::Instance().GetObjList())
			{
				if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj)) enemy->ApplyTypeParam();
			}
		}
		Help(U8("ここの数値は、敵を作った時に使う。押すと、もういる敵にも反映する（HP の残りの割合はそのまま）"));
	}

	if (ImGui::CollapsingHeader(U8("攻撃・動き")))
	{
		ImGui::PushID("EnemyAttack");
		DragI(U8("腕の攻撃のダメージ"), t.attackDamage, 0.2f, 0, 999, U8("普通の攻撃が当たった時のダメージ"));
		DragI(U8("着地の衝撃のダメージ"), t.smashDamage, 0.2f, 0, 999, U8("飛びかかり攻撃の着地で、円の中にいた時のダメージ"));
		DragI(U8("攻撃の後のすき（ザコ・中ボス）"), t.attackRecoverFrame, 0.5f, 0, 600, U8("攻撃の後、次の攻撃をしてこないフレーム数"));
		DragI(U8("攻撃の後のすき（ボス）"), t.bossAttackRecoverFrame, 0.5f, 0, 600, U8("ボスの攻撃の後、次の攻撃をしてこないフレーム数"));
		DragI(U8("すきのばらつき"), t.attackRecoverRandom, 0.5f, 0, 600, U8("すきに 0 ～ この値を足す（全員が同じリズムで殴ってこないように）"));
		DragI(U8("飛びかかりの後のすき"), t.jumpAttackRecoverFrame, 0.5f, 0, 600, U8("飛びかかり攻撃の後、次の攻撃をしてこないフレーム数"));
		DragI(U8("飛びかかる確率"), t.jumpAttackChance, 0.2f, 0, 100, U8("飛びかかれる距離にいる時、1 フレームに使う確率（％）"));
		Drag(U8("飛びかかる距離（近い方）"), t.jumpAttackMinDist, 0.1f, 0.0f, 50.0f, "%.1f m", U8("プレイヤーがこれより遠い時に飛びかかる"));
		Drag(U8("飛びかかる距離（遠い方）"), t.jumpAttackMaxDist, 0.1f, 0.0f, 50.0f, "%.1f m", U8("プレイヤーがこれより近い時に飛びかかる"));
		Drag(U8("追いかける速さ"), t.chaseSpeed, 0.002f, 0.0f, 1.0f, "%.3f", U8("1 フレームに進む距離（m）"));
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(U8("ノックバック（殴られた時に後ろへすべる）")))
	{
		ImGui::PushID("Knockback");
		Drag(U8("強さ"), t.knockbackSpeed, 0.002f, 0.0f, 1.0f, "%.3f", U8("殴られた瞬間の、1 フレームに進む距離。進む距離の合計 ＝ 強さ ÷ (1 − 止まり方)"));
		Drag(U8("止まり方"), t.knockbackDecay, 0.005f, 0.0f, 0.99f, "%.3f", U8("1 フレームごとに速さに掛ける。小さいほど早く止まる"));
		Drag(U8("クリティカルの倍率"), t.knockbackCriticalRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("クリティカルの時、強さに掛ける"));
		Drag(U8("攻撃中の倍率"), t.knockbackArmorRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("攻撃中（スーパーアーマー）の時"));
		Drag(U8("ATB の技を受けている時の倍率"), t.knockbackStunRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("技の途中で、届かなくならないように弱め"));
		Drag(U8("中ボスの倍率"), t.knockbackMidBossRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("中ボスは重いので飛びにくくする"));
		Drag(U8("ボスの倍率"), t.knockbackBossRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("ボスはもっと飛びにくくする"));
		ImGui::PopID();
	}

	if (ImGui::CollapsingHeader(U8("ブレイク"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox(U8("使う"), &b.breakEnable);
		ImGui::Checkbox(U8("ザコもブレイクする"), &b.normalBreak);
		ImGui::Checkbox(U8("中ボスもブレイクする"), &b.midBossBreak);
		Drag(U8("ゲージの最大"), b.breakMax, 1.0f, 1.0f, 1000.0f, "%.0f", U8("ここまでたまるとブレイクする"));

		ImGui::SeparatorText(U8("たまり方"));
		Drag(U8("普通の攻撃"), b.hitGain, 0.05f, 0.0f, 100.0f, "%.2f", U8("攻撃が 1 回当たるごとに、たまる量（二刀流は 1 振りで 2 回当たることがある）"));
		Drag(U8("クリティカルの倍率"), b.criticalRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("クリティカルの時、たまる量に掛ける"));
		Drag(U8("パニッシャーの倍率"), b.punisherRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("パニッシャーの攻撃の時、たまる量に掛ける"));
		Drag(U8("ATB の技の倍率"), b.abilityRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("ATB コマンドの技の時、たまる量に掛ける"));
		Drag(U8("回避の直後・パリィ"), b.bonusGain, 0.1f, 0.0f, 100.0f, "%.1f", U8("回避の直後・パリィの攻撃でたまる量（この機能はあとで作る。今は使っていない）"));

		ImGui::SeparatorText(U8("減り方"));
		Drag(U8("減り始めるまで"), b.decayDelaySec, 0.01f, 0.0f, 10.0f, "%.2f s", U8("最後に攻撃を当ててから、ゲージが減り始めるまでの時間"));
		Drag(U8("1 秒に減る量"), b.decayPerSec, 0.1f, 0.0f, 200.0f, "%.1f", U8("大きいほど速く減る。続けて当て続けないとたまらなくなる"));

		ImGui::SeparatorText(U8("ブレイク中"));
		Drag(U8("ブレイクの時間"), b.breakSec, 0.05f, 0.1f, 30.0f, "%.2f s", U8("ブレイクしている時間。この間にゲージが空になっていく"));
		Drag(U8("ダメージの倍率"), b.breakDamageRate, 0.05f, 1.0f, 10.0f, "%.2f", U8("ブレイク中に受けるダメージに掛ける"));

		if (ImGui::Button(U8("すべての敵をすぐブレイクさせる（テスト）")))
		{
			for (auto& obj : SceneManager::Instance().GetObjList())
			{
				if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj)) if (enemy->CanBreak()) enemy->AddBreak(b.breakMax);
			}
		}
	}

	if (ImGui::Button(U8("戦闘の設定を初期値に戻す"))) b = Battle();
	ImGui::PopID();
}

void StageSettings::DrawPlayerEditor()
{
	ImGui::PushID("Player");
	ImGui::Text(U8("今の攻撃の種類：0%d"), Player::s_currentAttackType);

	DrawLivePlayerEditor();

	PlayerTuning& t = m_player;
	if (ImGui::CollapsingHeader(U8("HP・攻撃力"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		DragI(U8("最大 HP（始めた時）"), t.maxHp, 1.0f, 1, 99999, U8("ゲームを始めた時の最大 HP。今の HP は上の「今のプレイヤー」で変える"));
		DragI(U8("攻撃力"), t.attackDamage, 0.2f, 0, 999, U8("1 回当たった時のダメージ（このあとモードの倍率を掛ける）"));
		DragI(U8("攻撃力（クリティカル）"), t.criticalAttackDamage, 0.2f, 0, 999, U8("クリティカルの時のダメージ（このあとモードの倍率を掛ける）"));
		DragI(U8("クリティカル率"), t.critRate, 0.2f, 0, 100, U8("1 振りごとに、クリティカルになる確率（％）"));
		Drag(U8("オペレーターのダメージの倍率"), t.operatorDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("オペレーターの攻撃のダメージに掛ける"));
		Drag(U8("パニッシャーのダメージの倍率"), t.punisherDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("パニッシャーの攻撃のダメージに掛ける"));
		Drag(U8("ATB の技のダメージの倍率"), t.abilityDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("ATB の技のダメージに掛ける（モードの倍率はかからない）"));
		DragI(U8("多段ヒットの間隔"), t.multiHitInterval, 0.2f, 1, 120, U8("同じ剣が同じ敵に、もう一度当たれるまでのフレーム数"));
		DragI(U8("ATB の技で、のけぞらせる長さ"), t.abilityStunFrame, 0.5f, 0, 600, U8("ATB の技が当たった敵が、のけぞり続けるフレーム数（当たるたびに延びる）"));
		ImGui::TextDisabled(U8("ダメージ ＝ 攻撃力 × モードの倍率（ブレイク中の敵には、さらにブレイクの倍率）"));
	}

	if (ImGui::CollapsingHeader(U8("モード")))
	{
		Drag(U8("オペレーターの攻撃の速さ"), t.operatorAttackSpeedRate, 0.01f, 0.1f, 5.0f, "%.2f", U8("攻撃のアニメーションの再生速度に掛ける"));
		Drag(U8("パニッシャーの攻撃の速さ"), t.punisherAttackSpeedRate, 0.01f, 0.1f, 5.0f, "%.2f", U8("攻撃のアニメーションの再生速度に掛ける"));
		Drag(U8("パニッシャーの移動の速さ"), t.punisherMoveRate, 0.01f, 0.1f, 5.0f, "%.2f", U8("オペレーターを 1.0 とした倍率"));
		Drag(U8("パニッシャーで受けるダメージ"), t.punisherTakenDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("パニッシャーの時に受けるダメージに掛ける"));
	}

	if (ImGui::CollapsingHeader(U8("受けるダメージ・ガード")))
	{
		DragI(U8("攻撃を受けた後の無敵"), t.damageInvincibleFrame, 0.5f, 0, 600, U8("攻撃を受けた後、次の攻撃を受けないフレーム数"));
		DragI(U8("大きくのけぞるダメージ"), t.heavyDamage, 0.2f, 0, 999, U8("これ以上のダメージを受けると、大きくのけぞる"));
		Drag(U8("回避中のダメージの倍率"), t.evadeDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("回避中に当たった時のダメージに掛ける"));
		Drag(U8("ガード中のダメージの倍率"), t.guardDamageRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("ガード中に受けるダメージに掛ける"));
		ImGui::SeparatorText(U8("盾の耐久値"));
		Drag(U8("耐久値の最大"), t.guardDurabilityMax, 1.0f, 1.0f, 9999.0f, "%.0f", U8("0 になるとガードを崩される"));
		Drag(U8("ダメージ 1 で減る量"), t.guardCostPerDamage, 0.05f, 0.0f, 100.0f, "%.2f", U8("受け止めたダメージ × これだけ減る"));
		Drag(U8("クリティカルで減る倍率"), t.guardCriticalCostRate, 0.05f, 0.0f, 10.0f, "%.2f", U8("クリティカルの攻撃を受け止めた時に掛ける"));
		DragI(U8("回復し始めるまで"), t.guardRegenDelay, 0.5f, 0, 600, U8("最後に受け止めてから、回復し始めるまでのフレーム数"));
		Drag(U8("1 フレームに回復する量"), t.guardRegenSpeed, 0.005f, 0.0f, 10.0f, "%.3f", U8("盾の耐久値が、1 フレームに回復する量"));
		DragI(U8("崩された後、ガードできない時間"), t.guardBreakFrame, 0.5f, 0, 600, U8("フレーム数（60 で 1 秒）"));
	}

	if (ImGui::CollapsingHeader(U8("ATB ゲージ・ロックオン")))
	{
		Drag(U8("当てた時にたまる量"), t.atbHitGain, 0.05f, 0.0f, 100.0f, "%.2f", U8("1 本 = 100。このあとモードの倍率を掛ける"));
		Drag(U8("オペレーターの倍率"), t.operatorAtbHitRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("オペレーターの時、たまる量に掛ける"));
		Drag(U8("パニッシャーの倍率"), t.punisherAtbHitRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("パニッシャーの時、たまる量に掛ける"));
		Drag(U8("クリティカルの倍率"), t.atbCriticalRate, 0.01f, 0.0f, 10.0f, "%.2f", U8("クリティカルの時、たまる量に掛ける"));
		Drag(U8("回避した時にたまる量"), t.atbEvadeGain, 0.1f, 0.0f, 100.0f, "%.1f", U8("回避するたびにたまる量"));
		Drag(U8("ロックオンの距離"), t.lockOnRange, 0.1f, 0.0f, 50.0f, "%.1f m", U8("この距離に近づいた敵に、自動でロックオンする"));
	}

	if (ImGui::CollapsingHeader(U8("動き"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		Drag(U8("回避の速さ"), PlayerParamManager::Instance().m_evadeSpeed, 0.002f, 0.01f, 0.5f, "%.3f", U8("回避の 1 フレームに進む距離（だんだん遅くなる）。PlayerParams.txt に保存する"));
		Drag(U8("走る速さの倍率"), Player::s_runSpeedRate, 0.01f, 1.0f, 2.5f, "%.2f", U8("1.0 でアニメーションの足の動きと同じ速さ"));
		ImGui::Checkbox(U8("ルートモーション"), &Player::s_useRootMotion);
		Help(U8("オン：アニメーションの体の動きどおりに進む（攻撃の踏み込みなど）"));
	}

	if (ImGui::CollapsingHeader(U8("剣"), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox(U8("攻撃判定・軌跡を剣の形に合わせる"), &Player::s_bladeShapeHitbox);
		Help(U8("オン：剣のモデルから刃の根元・先を求めて、攻撃判定の箱と軌跡を合わせる"));
		Drag(U8("投げる向きの微調整"), Player::s_throwAngleOffset, 0.5f, -90.0f, 90.0f, "%.1f deg", U8("剣を投げる攻撃（05 の 2 段目）で、剣の向きを足す角度"));

		ImGui::SeparatorText(U8("剣の持ち方（WeaponParams.json に保存する）"));
		ImGui::DragFloat3(U8("右の角度"), &Player::s_weaponRotR.x, 0.1f);
		ImGui::DragFloat3(U8("右の位置"), &Player::s_weaponPosR.x, 0.001f);
		ImGui::DragFloat3(U8("左の角度"), &Player::s_weaponRotL.x, 0.1f);
		ImGui::DragFloat3(U8("左の位置"), &Player::s_weaponPosL.x, 0.001f);
	}
	ImGui::PopID();
}

// ============================================================
// 今のプレイヤー・今いる敵（数値を直接変える。保存はしない）
// ============================================================
void StageSettings::DrawLivePlayerEditor()
{
	if (!ImGui::CollapsingHeader(U8("今のプレイヤー（保存しない）"), ImGuiTreeNodeFlags_DefaultOpen)) return;

	std::shared_ptr<Player> player = nullptr;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		player = std::dynamic_pointer_cast<Player>(obj);
		if (player) break;
	}
	if (!player)
	{
		ImGui::TextDisabled(U8("ゲームの画面にいる時だけ使える"));
		return;
	}

	int hp = player->GetHp();
	int maxHp = player->GetMaxHp();
	bool isChanged = false;
	isChanged |= ImGui::DragInt(U8("今の HP"), &hp, 1.0f, 1, maxHp, "%d", ImGuiSliderFlags_AlwaysClamp);
	isChanged |= ImGui::DragInt(U8("今の最大 HP"), &maxHp, 1.0f, 1, 99999, "%d", ImGuiSliderFlags_AlwaysClamp);
	if (isChanged) player->DebugSetHp(hp, maxHp);
	if (ImGui::Button(U8("全回復"))) player->DebugSetHp(player->GetMaxHp(), player->GetMaxHp());
	ImGui::SameLine();
	if (ImGui::Button(U8("HP を 1 にする"))) player->DebugSetHp(1, player->GetMaxHp());

	float atb = player->GetAtbGage();
	if (ImGui::DragFloat(U8("ATB ゲージ"), &atb, 1.0f, 0.0f, Player::kAtbBarMax * Player::kAtbBarNum, "%.0f", ImGuiSliderFlags_AlwaysClamp))
	{
		player->DebugSetAtb(atb);
	}
	Help(U8("1 本 = 100。2 本まで"));
}

void StageSettings::DrawLiveEnemyEditor()
{
	if (!ImGui::CollapsingHeader(U8("今いる敵（保存しない）"))) return;

	auto player = [&]() -> std::shared_ptr<Player>
	{
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (auto p = std::dynamic_pointer_cast<Player>(obj)) return p;
		}
		return nullptr;
	}();

	// 近い順に並べる（目の前の敵を見つけやすいように）
	std::vector<std::shared_ptr<Enemy>> enemies;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (auto enemy = std::dynamic_pointer_cast<Enemy>(obj); enemy && enemy->IsAlive()) enemies.push_back(enemy);
	}
	auto distOf = [&](const std::shared_ptr<Enemy>& e) { return player ? (e->GetPos() - player->GetPos()).Length() : 0.0f; };
	std::sort(enemies.begin(), enemies.end(), [&](const auto& a, const auto& b) { return distOf(a) < distOf(b); });

	if (enemies.empty())
	{
		ImGui::TextDisabled(U8("生きている敵がいない"));
		return;
	}

	for (size_t i = 0; i < enemies.size(); ++i)
	{
		const auto& enemy = enemies[i];
		const char* typeName = enemy->IsBoss() ? U8("ボス") : (enemy->IsMidBoss() ? U8("中ボス") : U8("ザコ"));
		ImGui::PushID(static_cast<int>(i));

		std::string label = std::string(typeName) + "  " + std::to_string(static_cast<int>(distOf(enemy))) + "m  HP "
			+ std::to_string(enemy->GetHp()) + " / " + std::to_string(enemy->GetMaxHp());
		if (ImGui::TreeNode(label.c_str()))
		{
			int hp = enemy->GetHp();
			if (ImGui::DragInt(U8("今の HP"), &hp, 1.0f, 1, enemy->GetMaxHp(), "%d", ImGuiSliderFlags_AlwaysClamp))
			{
				enemy->DebugSetHp(hp);
			}
			if (enemy->CanBreak())
			{
				float breakValue = enemy->GetBreakRatio() * m_battle.breakMax;
				if (enemy->IsBroken())
				{
					ImGui::Text(U8("ブレイク中（残り %.0f%%）"), enemy->GetBreakRatio() * 100.0f);
				}
				else if (ImGui::DragFloat(U8("ブレイクゲージ"), &breakValue, 0.5f, 0.0f, m_battle.breakMax, "%.1f", ImGuiSliderFlags_AlwaysClamp))
				{
					enemy->DebugSetBreak(breakValue);
				}
			}
			if (ImGui::Button(U8("HP を 1 にする"))) enemy->DebugSetHp(1);
			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}
