#pragma once

// 2D演出（タイトル画面など）で使う補助関数
namespace SpriteUtil
{
	// 現在の画面（ビューポート）の大きさを取得
	Math::Vector2 GetScreenSize();

	// 中心ほど明るい丸いぼかしのテクスチャを作成する（加算合成用。アルファは常に1）
	std::shared_ptr<KdTexture> CreateGlowTexture(int size);

	// 画面の四隅ほど暗くなるテクスチャを作成する（半透明合成用。黒色でアルファのみ変化）
	std::shared_ptr<KdTexture> CreateVignetteTexture(int width, int height);

	// 0～1の値を、終わりにかけてゆっくりになるように変換する
	inline float EaseOutCubic(float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		float inv = 1.0f - t;
		return 1.0f - inv * inv * inv;
	}

	// 0～1の範囲で滑らかに補間する
	inline float SmoothStep(float edge0, float edge1, float x)
	{
		float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}
