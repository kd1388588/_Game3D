#include "../../../Application/main.h"
#include "../../../Application/GameObject/Character/Player/Player.h"
#include "../../../Application/GameObject/Character/Player/PlayerParamManager.h"
#include "KdDebugGUI.h"

KdDebugGUI::KdDebugGUI()
{}
KdDebugGUI::~KdDebugGUI()
{ 
	GuiRelease(); 
}

void KdDebugGUI::GuiInit(int w, int h)
{
	// 初期化済みなら動作させない
	if (m_uqLog) return;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// Setup Dear ImGui style
	// ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();
	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(Application::Instance().GetWindowHandle(), ImVec2(w,h));
	ImGui_ImplDX11_Init(KdDirect3D::Instance().WorkDev(), KdDirect3D::Instance().WorkDevContext());

#include "imgui/ja_glyph_ranges.h"
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig config;
	config.MergeMode = true;
	io.Fonts->AddFontDefault();
	// 日本語対応
	io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\msgothic.ttc", 13.0f, &config, glyphRangesJapanese);
	m_uqLog = std::make_unique<ImGuiAppLog>();
}

void KdDebugGUI::GuiProcess()
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	//===========================================================
	// ImGui開始
	//===========================================================
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//===========================================================
	// 以下にImGui描画処理を記述
	//===========================================================

	// デバッグウィンドウ(日本語を表示したい場合はこう書く)
//	if (ImGui::Begin(U8("えふぴぃえす")))
//	{
		// FPS
//		ImGui::Text("FPS : %d", Application::Instance().GetNowFPS());
//	}
// 
	// =========================================================
	// Playerパラメータエディタ
	// =========================================================
	ImGui::SetNextWindowPos(ImVec2(850, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

	ImGui::Begin("Player Parameter Editor");

	ImGui::Text("Current Attack Pattern : 0%d", Player::s_currentAttackType);
	ImGui::Separator();

	// 上部に共通のセーブ・ロードボタン
	if (ImGui::Button("Save Params")) {
		PlayerParamManager::Instance().Save("PlayerParams.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Params")) {
		PlayerParamManager::Instance().Load("PlayerParams.txt");
	}
	ImGui::Separator();

	// タブ機能の開始
	if (ImGui::BeginTabBar("PlayerParamsTabBar"))
	{
		// ---------------------------------------------------
		// 攻撃アニメーション（Attack Params）
		// ---------------------------------------------------
		if (ImGui::BeginTabItem("Attack Params"))
		{
			auto& attackMap = PlayerParamManager::Instance().GetAttackMap();
			for (auto& pair : attackMap)
			{
				if (ImGui::TreeNode(pair.first.c_str()))
				{
					ImGui::DragFloat("Hit Start", &pair.second.hitStartFrame, 0.5f, 0.0f, 100.0f);
					ImGui::DragFloat("Hit End", &pair.second.hitEndFrame, 0.5f, 0.0f, 100.0f);
					ImGui::DragFloat("Cancel", &pair.second.cancelFrame, 0.5f, 0.0f, 999.0f);
					ImGui::TreePop();
				}
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Animation Move"))
		{
			ImGui::SliderFloat("Evade Speed", &PlayerParamManager::Instance().m_evadeSpeed, 0.01f, 0.5f);
			ImGui::SliderFloat("Dash Speed", &PlayerParamManager::Instance().m_dashSpeed, 0.01f, 0.5f);
			ImGui::SliderFloat("Attack Step", &PlayerParamManager::Instance().m_attackRootScale, 0.001f, 0.5f);

			if (ImGui::Button("Save Params"))
			{
				PlayerParamManager::Instance().Save("Asset/Data/PlayerParams.txt");
			}

			ImGui::EndTabItem();
		}

		// ---------------------------------------------------
		// 【タブ3】武器位置調整エディタ（Weapon Adjust）
		// ---------------------------------------------------
		if (ImGui::BeginTabItem("Weapon Adjust"))
		{
			// ▼ JSON セーブ・ロードボタン ▼
			if (ImGui::Button("Save JSON (.json)"))
			{
				PlayerParamManager::Instance().SaveWeaponParams("Asset/Data/WeaponParams.json");
			}
			ImGui::SameLine();
			if (ImGui::Button("Load JSON (.json)"))
			{
				PlayerParamManager::Instance().LoadWeaponParams("Asset/Data/WeaponParams.json");
			}
			ImGui::Separator();

			// Posの微調整は「0.005f」、Rot（角度）は「0.5f」単位で動かすとやりやすいです
			ImGui::DragFloat3("Rotation (Right)", &Player::s_weaponRotR.x, 0.1f);
			ImGui::DragFloat3("Position (Right)", &Player::s_weaponPosR.x, 0.001f);
			ImGui::DragFloat3("Rotation (Left)", &Player::s_weaponRotL.x, 0.1f);
			ImGui::DragFloat3("Position (Left)", &Player::s_weaponPosL.x, 0.001f);

			ImGui::Separator();
			ImGui::Text("--- Sheathed (Back) ---");

			ImGui::DragFloat3("Sheath Rot (R)", &Player::s_sheathedRotR.x, 0.1f);
			ImGui::DragFloat3("Sheath Pos (R)", &Player::s_sheathedPosR.x, 0.001f);
			ImGui::DragFloat3("Sheath Rot (L)", &Player::s_sheathedRotL.x, 0.1f);
			ImGui::DragFloat3("Sheath Pos (L)", &Player::s_sheathedPosL.x, 0.001f);

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();

	// ログウィンドウ
	// m_uqLog->Draw("Log Window");

	//=====================================================
	// ログ出力 ・・・ AddLog("～") で追加
	//=====================================================

//	m_uqLog->AddLog("hello world\n");

	//=====================================================
	// 別ソースファイルからログを出力する場合
	//=====================================================

//	KdDebugGUI::Instance().AddLog("TestLog\n");

	//===========================================================
	// ここより上にImGuiの描画はする事
	//===========================================================
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void KdDebugGUI::AddLog(const char* fmt,...)
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	char tmpStr[128] = {};
	va_list args;
	va_start(args, fmt);
	vsprintf_s(tmpStr, fmt, args);
	m_uqLog->AddLog(tmpStr);
	va_end(args);
}

void KdDebugGUI::ClearLog()
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	m_uqLog->Clear();
}

void KdDebugGUI::GuiRelease()
{
	// 初期化されてないなら動作させない
	if (!m_uqLog) return;

	m_uqLog = nullptr;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
