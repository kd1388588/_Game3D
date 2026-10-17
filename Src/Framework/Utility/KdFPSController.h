#pragma once

//============================================================
// アプリケーションのFPS制御 + 測定
//============================================================
struct KdFPSController
{
	// FPS制御
	int		m_nowfps = 0;		// 現在のFPS値
	int		m_maxFps = 60;		// 最大FPS

	void Init();

	void UpdateStartTime();

	void Update();

	float GetDeltaTime() const { return m_DeltaTime; }

private:

	void Control();

	void Monitoring();

	DWORD		m_frameStartTime = 0;		// フレームの開始時間

	// 細かい時間（マイクロ秒単位）でフレームの間隔を合わせる
	// （Sleep だけだと 1ミリ秒単位で長く待ってしまい、60 ではなく 58～59 FPS になるため）
	LARGE_INTEGER	m_qpcFrequency = {};		// 1秒あたりのカウント数
	LARGE_INTEGER	m_nextFrameTime = {};		// 次のフレームを始める時刻

	int			m_fpsCnt = 0;				// FPS計測用カウント
	DWORD		m_fpsMonitorBeginTime = 0;	// FPS計測開始時間

	float		m_DeltaTime = 0;			// デルタタイム

	const int	kSecond = 1000;				// １秒のミリ秒
};
