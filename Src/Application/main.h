#pragma once

//============================================================
// アプリケーションクラス
//	Application::Instance().～ でどこからでもアクセス可能
//	（元のテンプレートには「APP.～」と書いてあったが、APP という名前は定義されていない）
//
// 【役割】ゲーム全体を動かすクラス
//         ウィンドウ・FPS の管理と、ゲームループ（更新 → 描画 の繰り返し）を持つ
//
// 【構成】Execute() がゲームループ。1 フレームの中で、次の順に呼ぶ（main.cpp）
//           KdBeginUpdate → PreUpdate → Update → PostUpdate → KdPostUpdate
//           KdBeginDraw   → PreDraw  → Draw   → PostDraw   → DrawSprite → KdPostDraw
//         「Kd」が付く関数はフレームワークの準備・後片付け、
//         付かない関数はゲームの処理（SceneManager に任せる）
//
// 【なぜ】更新を Pre / 普通 / Post の 3 つに分けるのは、順番が大事な処理があるから
//         例：全員が動き終わった後（PostUpdate）に、カメラの位置を決める
//             （先にカメラを決めると、1 フレーム前のキャラの位置を映してしまう）
//
// 【参考】Game Programming Patterns「Game Loop」（ゲームループの考え方）
//         https://gameprogrammingpatterns.com/game-loop.html
//============================================================
class Application
{
// メンバ
public:

	// アプリケーション実行
	void Execute();

	// アプリケーション終了
	void End()							{ m_endFlag = true; }

	HWND GetWindowHandle()		const	{ return m_window.GetWndHandle(); }
	int GetMouseWheelValue()	const	{ return m_window.GetMouseWheelVal(); }

	int		GetNowFPS()			const	{ return m_fpsController.m_nowfps; }
	int		GetMaxFPS()			const	{ return m_fpsController.m_maxFps; }
	float	GetDeltaTime()		const	{ return m_fpsController.GetDeltaTime(); }
private:

	void KdBeginUpdate();
	void PreUpdate();
	void Update();
	void PostUpdate();

	// F8 でデバッグ表示を切り替え、シーンに合わせてマウスカーソルを出す・消す
	// （F12 は Visual Studio のデバッグ中に「止める」キーなので使わない）
	void UpdateDebugToggle();
	void SetCursorVisible(bool isVisible);
	void KdPostUpdate();

	void KdBeginDraw(bool usePostProcess = true);
	void PreDraw();
	void Draw();
	void PostDraw();
	void DrawSprite();
	void KdPostDraw();

	// アプリケーション初期化
	bool Init(int w, int h);

	// アプリケーション解放
	void Release();

	// ゲームウィンドウクラス
	KdWindow		m_window;

	// FPSコントローラー
	KdFPSController	m_fpsController;

	// ゲーム終了フラグ trueで終了する
	bool		m_endFlag = false;

	// マウスカーソルを出しているか
	bool		m_isCursorVisible = true;

//=====================================================
// シングルトンパターン
// 【なぜ】アプリケーションはゲームに 1 つだけで、どこからでも使いたい
//         （例：KdDebugGUI がウィンドウのハンドルを、TitleScene が End() を使う）
// 【しくみ】関数の中の static 変数は、最初に呼ばれた時に 1 回だけ作られ、
//           プログラムが終わるまで残る（「Meyers のシングルトン」と呼ばれる書き方）
//           コンストラクタを private にして、ほかの所から 2 つ目を作れないようにする
// 【注意】便利だが、何でもシングルトンにすると「どこから変えられたか」が追いにくくなる
// 【参考】Game Programming Patterns「Singleton」
//         https://gameprogrammingpatterns.com/singleton.html
//=====================================================
private:
	// 外から作れないように、コンストラクタを private にする
	Application() {}

public:
	static Application &Instance(){
		static Application Instance;
		return Instance;
	}
};
