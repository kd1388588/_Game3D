#pragma once

// ============================================================
// ヒットストップ（攻撃が当たった瞬間に、キャラの動きを数フレームだけ止めて「手ごたえ」を出す）
// ・ゲーム全体で1つだけ（攻撃した側も、された側も一緒に止まる）
// ・止まっている間は、カメラを少しだけ揺らす
// ・使い方：当たった時に HitStop::Request(止めるフレーム数, 揺れの強さ) を呼ぶだけ
//
// 【構成】・Request    ：止める（プレイヤーの攻撃 AttackOBB・敵の攻撃を受けた時 OnDamage など）
//         ・IsStopping ：Player・Enemy の Update の最初で見て、止まっている間は動かさない
//         ・Advance    ：1 フレーム進める（Player::PostUpdate で、1 フレームに 1 回だけ）
//         ・GetShakePower：TPSCamera が見て、カメラを揺らす
// 【なぜ Request で長い方・強い方を使うのか】
//   同じフレームに何体にも当たると、何回も Request される。足し算にすると止まる時間が長くなりすぎる
// 【なぜ揺れをだんだん弱くするのか】残りのフレーム ÷ 始めのフレーム をかけると、
//   当たった瞬間が一番強く、自然に収まっていく（急に止まると、カクッとして見える）
// 【なぜ GameTime と分けるのか】スロー（GameTime）は「ゆっくり動く」、ヒットストップは「完全に止まる」で、
//   止めている間もカメラは揺らしたいので、別の仕組みにしている
// 【参考】ヒットストップ（ゲームの手ごたえ）についての考え方
//   Steve Swink『Game Feel』（ゲームの「手ごたえ」を扱った本）
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
