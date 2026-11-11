#include "KdFPSController.h"

// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
// FPSの制御コントローラー
// ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### ##### #####
void KdFPSController::Init()
{
	m_fpsMonitorBeginTime = timeGetTime();

	// 【BRAVER で足した所】細かい時計の「1 秒あたりのカウント数」と「今の時刻」を読む
	QueryPerformanceFrequency(&m_qpcFrequency);
	QueryPerformanceCounter(&m_nextFrameTime);
}

void KdFPSController::UpdateStartTime()
{
	m_frameStartTime = timeGetTime();
}

void KdFPSController::Update()
{
	Control();

	// デルタ時間の計算
	DWORD _currentTime = timeGetTime();
	m_DeltaTime = (_currentTime - m_frameStartTime) / static_cast<float>(kSecond);

	Monitoring();
}

// FPS制御
// ============================================================
// 【BRAVER で書きかえた所】元は「処理にかかった時間を引いた分だけ Sleep する」形だった
// 【役割】1 フレームが 1/60 秒になるように待つ
// 【しくみ】「次のフレームを始める予定の時刻」を決めておき、その時刻まで待つ
// 【なぜ予定の時刻を使うのか】
//   「今から 1/60 秒待つ」だと、待ちすぎた分が毎フレームたまり、だんだん遅れていく
//   予定の時刻を 1/60 秒ずつ進めれば、少し待ちすぎても次のフレームで取り戻せる
// 【なぜ Sleep(1) と Sleep(0) を使い分けるのか】
//   Sleep(1) は CPU を休ませられるが、1 ミリ秒より長く（2 ミリ秒近く）待ってしまうことがある
//   残りが 2 ミリ秒を切ったら Sleep(0)（すぐ戻る）で細かく待ち、待ちすぎを防ぐ
//   元の形では待ちすぎて 58～59 FPS になっていたので、60 にするために書きかえた
// 【参考】高い精度の時刻（QueryPerformanceCounter）
//   https://learn.microsoft.com/ja-jp/windows/win32/sysinfo/acquiring-high-resolution-time-stamps
//   Sleep https://learn.microsoft.com/ja-jp/windows/win32/api/synchapi/nf-synchapi-sleep
// ============================================================
void KdFPSController::Control()
{
	if (m_qpcFrequency.QuadPart <= 0 || m_maxFps <= 0) return;

	// 1フレームの長さ（カウント数）。次のフレームの開始時刻を、前の予定時刻から1フレーム分進める
	// （「今から1フレーム待つ」と待ちすぎた分がたまっていくので、予定の時刻に合わせる）
	const LONGLONG ticksPerFrame = m_qpcFrequency.QuadPart / m_maxFps;
	m_nextFrameTime.QuadPart += ticksPerFrame;

	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);

	// 処理が大きく遅れた時（読み込み・ウィンドウの移動など）は、今を基準にし直す
	// （遅れを取り戻そうとして、何フレームも待たずに進んでしまわないように）
	if (now.QuadPart > m_nextFrameTime.QuadPart + ticksPerFrame)
	{
		m_nextFrameTime = now;
		return;
	}

	// 予定の時刻まで待つ：残りが 2ミリ秒以上ある間は Sleep で休み、最後は細かく待つ
	const LONGLONG sleepThreshold = m_qpcFrequency.QuadPart / 500;	// 2ミリ秒
	while (true)
	{
		QueryPerformanceCounter(&now);
		LONGLONG remain = m_nextFrameTime.QuadPart - now.QuadPart;
		if (remain <= 0) break;

		if (remain > sleepThreshold)	Sleep(1);
		else							Sleep(0);
	}
}

// 現在のFPS計測
void KdFPSController::Monitoring()
{
	// FPS計測のタイミング　0.5秒おき
	constexpr float kFpsRefreshFrame = 500;		

	m_fpsCnt++;

	// 0.5秒おきに FPS計測
	if (m_frameStartTime - m_fpsMonitorBeginTime >= kFpsRefreshFrame)
	{
		// 現在のFPS算出
		// 【BRAVER で書きかえた所】「＋ 割る数の半分」をしてから割ると四捨五入になる
		// 四捨五入する（切り捨てだと、ぴったり60でも 59 と表示されることがあるため）
		DWORD elapsed = m_frameStartTime - m_fpsMonitorBeginTime;
		m_nowfps = static_cast<int>((m_fpsCnt * kSecond + elapsed / 2) / elapsed);

		m_fpsMonitorBeginTime = m_frameStartTime;

		m_fpsCnt = 0;
	}
}
