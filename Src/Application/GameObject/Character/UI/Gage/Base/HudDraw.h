#pragma once

// ============================================================
// FF7R 風の HUD を描く部品（斜めの形）
// ・2D の四角（DrawBox）を、行列で横に傾けて平行四辺形にする（画像はいらない）
// ・座標は画面の真ん中が原点、右が +X、上が +Y（1280x720）
// ・slant：高さ 1 あたり、右へずらす量（0.27 で約 15 度、0.8 で約 39 度傾く）
// ・pivotY：傾ける基準の高さ（この高さでは左右にずれない）。同じ形の部品は同じ pivotY にすると、つながって見える
// ============================================================
namespace HudDraw
{
	constexpr float kPanelSlant = 0.27f;	// パネルの傾き
	constexpr float kGaugeSlant = 0.8f;		// ゲージの傾き（パネルより強く傾ける）

	// 傾いた四角を1つ描く（cx, cy：中心、hw, hh：幅・高さの半分）
	inline void SlantRect(float cx, float cy, float hw, float hh, const Math::Color& color, float slant, float pivotY, bool fill = true)
	{
		auto& sprite = KdShaderManager::Instance().m_spriteShader;
		Math::Matrix m = Math::Matrix::Identity;
		m._21 = slant;		// y が上がるほど x を右へずらす
		m._41 = cx;
		m._42 = pivotY;
		sprite.SetMatrix(m);
		sprite.DrawBox(0, static_cast<int>(std::lround(cy - pivotY)), static_cast<int>(hw), static_cast<int>(hh), &color, fill);
		sprite.SetMatrix(Math::Matrix::Identity);
	}

	// 左から右へ色が変わる、傾いた帯（細い帯を並べてグラデーションにする）
	inline void SlantGradient(float cx, float cy, float hw, float hh, const Math::Color& left, const Math::Color& right,
		float slant, float pivotY, int steps = 16)
	{
		float stepW = hw * 2.0f / steps;
		for (int i = 0; i < steps; ++i)
		{
			float t = (i + 0.5f) / steps;
			float x = cx - hw + stepW * (i + 0.5f);
			// 帯と帯のすき間が出ないように、少しだけ重ねる
			SlantRect(x, cy, stepW * 0.5f + 0.6f, hh, Math::Color::Lerp(left, right, t), slant, pivotY);
		}
	}

	// 傾いたゲージ（left：左端、ratio：たまっている割合 0～1）
	inline void SlantGauge(float left, float cy, float width, float height, float ratio,
		const Math::Color& fill, const Math::Color& back, float slant = kGaugeSlant)
	{
		float hh = height * 0.5f;
		SlantRect(left + width * 0.5f, cy, width * 0.5f, hh, back, slant, cy);
		float w = width * std::clamp(ratio, 0.0f, 1.0f);
		if (w >= 1.0f) SlantRect(left + w * 0.5f, cy, w * 0.5f, hh, fill, slant, cy);
	}

	// 色の明るさを、周期的に上げ下げする（満タン・点滅用。wave は 0～1）
	inline Math::Color Brighten(const Math::Color& c, float amount)
	{
		return { std::min(c.x + amount, 1.0f), std::min(c.y + amount, 1.0f), std::min(c.z + amount, 1.0f), c.w };
	}
}
