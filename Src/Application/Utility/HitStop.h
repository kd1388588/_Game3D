#pragma once

// ============================================================
// ヒットストップ（攻撃が当たった瞬間に、キャラの動きを数フレームだけ止めて「手ごたえ」を出す）
// ・ゲーム全体で1つだけ（攻撃した側も、された側も一緒に止まる）
// ・止まっている間は、カメラを少しだけ揺らす
// ・使い方：当たった時に HitStop::Request(止めるフレーム数, 揺れの強さ) を呼ぶだけ
// ============================================================
class HitStop
{
public:

	// ヒットストップを始める（すでに止まっている時は、長い方・強い方を使う）
	static void Request(int frames, float shakePower = 0.0f)
	{
		if (frames <= 0) return;
		s_timer = std::max(s_timer, frames);
		s_maxTimer = std::max(s_maxTimer, frames);
		s_shakePower = std::max(s_shakePower, shakePower);
	}

	// 止まっている途中か（キャラの Update の最初でこれを見て、true なら動かさない）
	static bool IsStopping() { return s_timer > 0; }

	// 時間を1フレーム進める（すべてのキャラの Update が終わった後に、1フレームに1回だけ呼ぶ）
	static void Advance()
	{
		if (s_timer <= 0) return;

		s_timer--;
		if (s_timer <= 0) Reset();
	}

	// カメラの揺れの強さ（止まり始めが一番強く、だんだん弱くなる）
	static float GetShakePower()
	{
		if (s_timer <= 0 || s_maxTimer <= 0) return 0.0f;
		return s_shakePower * static_cast<float>(s_timer) / static_cast<float>(s_maxTimer);
	}

	// すべて止める（シーンの切り替え時など）
	static void Reset()
	{
		s_timer = 0;
		s_maxTimer = 0;
		s_shakePower = 0.0f;
	}

private:

	inline static int	s_timer			= 0;		// 残りフレーム数
	inline static int	s_maxTimer		= 0;		// 始めた時のフレーム数（揺れを弱めていくのに使う）
	inline static float	s_shakePower	= 0.0f;		// カメラの揺れの強さ
};
