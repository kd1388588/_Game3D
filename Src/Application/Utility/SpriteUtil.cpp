#include "SpriteUtil.h"

namespace
{
	// RGBA(0～1) を R8G8B8A8 の1ピクセルに変換
	UINT ToPixel(float r, float g, float b, float a)
	{
		UINT ur = static_cast<UINT>(std::clamp(r, 0.0f, 1.0f) * 255.0f);
		UINT ug = static_cast<UINT>(std::clamp(g, 0.0f, 1.0f) * 255.0f);
		UINT ub = static_cast<UINT>(std::clamp(b, 0.0f, 1.0f) * 255.0f);
		UINT ua = static_cast<UINT>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
		return (ua << 24) | (ub << 16) | (ug << 8) | ur;
	}

	// ピクセルの配列からテクスチャを作成
	std::shared_ptr<KdTexture> CreateTexture(int width, int height, const std::vector<UINT>& pixels)
	{
		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = pixels.data();
		initData.SysMemPitch = width * sizeof(UINT);

		auto tex = std::make_shared<KdTexture>();
		if (!tex->Create(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 1, &initData))
		{
			OutputDebugStringA("【演出用テクスチャの作成に失敗】\n");
			return nullptr;
		}
		return tex;
	}

	// スプライトシェーダーはテクスチャのアルファが0.1未満だと描画しないので、
	// 半透明のテクスチャはこれ以上のアルファにしておく
	constexpr float kMinSpriteAlpha = 0.1f;
}

Math::Vector2 SpriteUtil::GetScreenSize()
{
	UINT numViewports = 1;
	D3D11_VIEWPORT vp = {};
	KdDirect3D::Instance().WorkDevContext()->RSGetViewports(&numViewports, &vp);

	// まだビューポートが設定されていない場合は標準の大きさ
	if (numViewports == 0 || vp.Width <= 0.0f || vp.Height <= 0.0f)
	{
		return { 1280.0f, 720.0f };
	}
	return { vp.Width, vp.Height };
}

std::shared_ptr<KdTexture> SpriteUtil::CreateGlowTexture(int size)
{
	std::vector<UINT> pixels(size * size);

	for (int y = 0; y < size; ++y)
	{
		for (int x = 0; x < size; ++x)
		{
			// 中心からの距離（0：中心、1：ふち）
			float dx = (x + 0.5f) / size * 2.0f - 1.0f;
			float dy = (y + 0.5f) / size * 2.0f - 1.0f;
			float dist = std::sqrt(dx * dx + dy * dy);

			// 中心ほど明るく、ふちに向かってなめらかに消える
			float bright = std::pow(SmoothStep(1.0f, 0.0f, dist), 2.0f);

			// 加算合成で描くので、明るさを色で表現する（アルファは常に1）
			pixels[y * size + x] = ToPixel(bright, bright, bright, 1.0f);
		}
	}

	return CreateTexture(size, size, pixels);
}

std::shared_ptr<KdTexture> SpriteUtil::CreateVignetteTexture(int width, int height)
{
	std::vector<UINT> pixels(width * height);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			// 画面中央からの距離（0：中央、1：上下左右の端、約1.4：四隅）
			float dx = (x + 0.5f) / width * 2.0f - 1.0f;
			float dy = (y + 0.5f) / height * 2.0f - 1.0f;
			float dist = std::sqrt(dx * dx + dy * dy);

			float dark = SmoothStep(0.4f, 1.45f, dist);
			float alpha = kMinSpriteAlpha + (1.0f - kMinSpriteAlpha) * dark;
			pixels[y * width + x] = ToPixel(0.0f, 0.0f, 0.0f, alpha);
		}
	}

	return CreateTexture(width, height, pixels);
}
