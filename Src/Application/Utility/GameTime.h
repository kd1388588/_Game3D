#pragma once

// ============================================================
// ゲームの時間の速さ（スローモーション用）
// ・1.0 で普通の速さ、0.1 なら 10分の1 の速さ（コマンドメニューを開いている間など）
// ・キャラのアニメーション・重力・エフェクトがこの速さで動く
// ============================================================
class GameTime
{
public:

	// 時間の速さを変える
	static void SetScale(float scale)
	{
		s_scale = std::clamp(scale, 0.0f, 1.0f);
		KdEffekseerManager::GetInstance().SetUpdateSpeed(s_scale);
	}

	static float GetScale() { return s_scale; }

	// スローモーション中か（キャラの考える処理（攻撃するか・どこへ進むか）を止めるのに使う）
	static bool IsSlow() { return s_scale < 1.0f; }

	// 普通の速さに戻す（シーンの切り替え時など）
	static void Reset() { SetScale(1.0f); }

private:

	inline static float s_scale = 1.0f;
};
