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
	};

	UIText() {}
	~UIText() override {}

	void Update() override;
	void DrawSprite() override;

	// 表示する文字列を設定（半角英数字のみ）
	void SetText(const std::string& text, FontSize size = FontSize::Medium);

	// 文字列の中心の位置を設定（画面中央が原点、上が+Y）
	void SetCenterPos(const Math::Vector2& pos) { m_centerPos = pos; }

	void SetColor(const Math::Color& color) { m_color = color; }

	// 点滅させるかどうか
	void SetBlink(bool isBlink) { m_isBlink = isBlink; m_blinkTimer = 0; }

private:

	// 使うフォントをフォントマネージャーに登録する（最初の1回だけ）
	static void RegisterFonts();

	// 文字の大きさに対応するフォント番号を取得
	static int GetFontNo(FontSize size);

	std::shared_ptr<KdFontSprite>	m_fontSprite = nullptr;
	Math::Vector2					m_centerPos = Math::Vector2::Zero;
	Math::Color						m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool							m_isBlink = false;
	int								m_blinkTimer = 0;
};
