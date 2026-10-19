#pragma once

// 画面に文字を表示するUI（中央揃え・点滅に対応）
class UIText : public KdGameObject
{
public:

	// 文字の大きさ
	enum class FontSize
	{
		Small,
		Medium,
		Large,
		Title,		// タイトルロゴ用（特大）

		// 英数字だけの文字に使う（オリジナルフォント「Braver Stone」。日本語は表示できない）
		LabelSmall,
		LabelMedium,
	};

	UIText() {}
	~UIText() override {}

	void Update() override;
	void DrawSprite() override;

	// 表示する文字列を設定（日本語は Small / Medium / Large のみ。Title のフォントは英数字のみ）
	void SetText(const std::string& text, FontSize size = FontSize::Medium);

	// 文字列の中心の位置を設定（画面中央が原点、上が+Y）
	void SetCenterPos(const Math::Vector2& pos) { m_centerPos = pos; }

	void SetColor(const Math::Color& color) { m_color = color; }
	void SetAlpha(float alpha) { m_color.w = alpha; }

	// 文字の拡大率（中心を基準に拡大・縮小する）
	void SetScale(float scale) { m_scale = scale; }

	// 拡大前の文字列の大きさ（ピクセル）
	Math::Vector2 GetSize() const;

	// 点滅させるかどうか
	void SetBlink(bool isBlink) { m_isBlink = isBlink; m_blinkTimer = 0; }

	// 文字の周りに暗いふちを付けるかどうか（最初は付ける）
	void SetOutline(bool isOutline) { m_isOutline = isOutline; }

private:

	// 使うフォントをフォントマネージャーに登録する（最初の1回だけ）
	static void RegisterFonts();

	// 文字の大きさに対応するフォント番号を取得
	static int GetFontNo(FontSize size);

	std::shared_ptr<KdFontSprite>	m_fontSprite = nullptr;
	Math::Vector2					m_centerPos = Math::Vector2::Zero;
	Math::Color						m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float							m_scale = 1.0f;
	bool							m_isBlink = false;
	bool							m_isOutline = true;
	int								m_blinkTimer = 0;
};
