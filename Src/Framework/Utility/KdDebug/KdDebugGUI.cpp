#include "../../../Application/main.h"
#include "../../../Application/Utility/StageSettings.h"
#include "../../../Application/Utility/EffectSettings.h"
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

	// 【BRAVER で足した所】デバッグ表示を消している時は、ウィンドウを何も出さない
	// 【なぜ Render を呼んでから return するのか】
	//   ImGui は「NewFrame を呼んだら、そのフレームの中で必ず Render まで呼ぶ」決まりになっている
	//   途中で抜けると、次のフレームの NewFrame でエラー（assert）になる
	// 【参考】Dear ImGui https://github.com/ocornut/imgui （NewFrame → ウィンドウを作る → Render の流れ）
	if (!m_isVisible)
	{
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		return;
	}

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
	// 【BRAVER で変えた所】前はここに「Player Parameter Editor」のウィンドウがあった
	//   使っていない項目（Attack Params）や、保存先が違うボタンがあったので消し、
	//   残りの項目は Object Editor の「Player」タブに移した（StageSettings::DrawPlayerEditor）
	// =========================================================

	// オブジェクトエディタ（Player / Enemy / Stage / UI のタブ。ゲームを動かしたまま調整する）
	StageSettings::Instance().DrawEditor();

	// エフェクトエディタ（エフェクトの大きさ・速さ・色（モードの色）・Effekseer エディタとの同期）
	EffectSettings::Instance().DrawEditor();

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
