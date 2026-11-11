#pragma once

// 2D演出（タイトル画面など）で使う補助関数
//
// 【役割】2D の演出に使う画像を、ファイルではなくプログラムで作る。なめらかに変化させる計算の関数
// 【使う所】TitleEmber（光の玉）、TitleUI（光の玉・画面のふちを暗くする画像・登場の動き）
// 【なぜ画像をプログラムで作るのか】丸いぼかしや周辺減光は、数式で正確に作れて、大きさも自由に変えられる
//         画像ファイルを用意しなくてよい
// 【参考】イージング関数（EaseOutCubic など）https://easings.net/ja
//         smoothstep（HLSL にも同じ名前の関数がある）
//         https://learn.microsoft.com/ja-jp/windows/win32/direct3dhlsl/dx-graphics-hlsl-smoothstep
namespace SpriteUtil
{
	// 現在の画面（ビューポート）の大きさを取得
	Math::Vector2 GetScreenSize();

	// 中心ほど明るい丸いぼかしのテクスチャを作成する（加算合成用。アルファは常に1）
	std::shared_ptr<KdTexture> CreateGlowTexture(int size);

	// 画面の四隅ほど暗くなるテクスチャを作成する（半透明合成用。黒色でアルファのみ変化）
	std::shared_ptr<KdTexture> CreateVignetteTexture(int width, int height);

	// 0～1の値を、終わりにかけてゆっくりになるように変換する
	// （1 - (1 - t)^3。最初は速く動いて、止まる前にゆっくりになる。物が止まる時の自然な動き）
	inline float EaseOutCubic(float t)
	{
		t = std::clamp(t, 0.0f, 1.0f);
		float inv = 1.0f - t;
		return 1.0f - inv * inv * inv;
	}

	// 0～1の範囲で滑らかに補間する
	// （t * t * (3 - 2t)。始めと終わりの傾きが 0 なので、つなぎ目が目立たない）
	inline float SmoothStep(float edge0, float edge1, float x)
	{
		float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
}
