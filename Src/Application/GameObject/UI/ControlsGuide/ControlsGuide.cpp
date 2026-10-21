#include "ControlsGuide.h"
#include "../Text/UIText.h"
#include "../../Character/Player/Player.h"

namespace
{
	// 画面の右下（画面中央が原点、右が+X・上が+Y。画面は 1280x720）
	constexpr float		kRightX			= 625.0f;	// パネルの右端
	constexpr float		kBottomY		= -345.0f;	// パネルの下端
	constexpr float		kPanelWidth		= 250.0f;
	constexpr float		kPadding		= 10.0f;	// パネルの内側の余白
	constexpr float		kRowSpacing		= 20.0f;	// 行と行の間
	constexpr float		kTextScale		= 0.75f;	// 文字の大きさ（Small の 24px を 18px にする）
	constexpr float		kActionOffsetX	= 105.0f;	// キーの左端から、操作の左端までの距離
	constexpr float		kFadeSpeed		= 0.1f;		// 出たり消えたりする速さ

	const Math::Color	kPanelColor		= { 0.02f, 0.03f, 0.08f, 0.55f };
	const Math::Color	kKeyColor		= { 1.0f, 0.85f, 0.4f, 1.0f };	// キーの名前（タイトルの操作説明と同じような金色）
	const Math::Color	kActionColor	= { 0.9f, 0.9f, 0.9f, 1.0f };

	const std::vector<std::pair<std::string, std::string>> kControls =
	{
		{ "WASD",			"移動" },
		{ "左クリック",		"攻撃" },
		{ "右クリック",		"回避" },
		{ "Shift",			"ダッシュ / ガード" },
		{ "E",				"モード切り替え" },
		{ "F",				"ATBコマンド" },
	};
}

void ControlsGuide::Init()
{
	float panelLeft = kRightX - kPanelWidth;
	float keyLeft = panelLeft + kPadding;
	float topRowY = kBottomY + kPadding + kRowSpacing * (static_cast<float>(kControls.size()) - 0.5f);

	for (size_t i = 0; i < kControls.size(); ++i)
	{
		float y = topRowY - kRowSpacing * static_cast<float>(i);

		// UIText は中心の位置で表示するので、左端の位置に文字の幅の半分を足して左揃えにする
		Row row;
		row.key = std::make_shared<UIText>();
		row.key->SetText(kControls[i].first, UIText::FontSize::Small);
		row.key->SetScale(kTextScale);
		row.key->SetCenterPos({ keyLeft + row.key->GetSize().x * kTextScale * 0.5f, y });

		row.action = std::make_shared<UIText>();
		row.action->SetText(kControls[i].second, UIText::FontSize::Small);
		row.action->SetScale(kTextScale);
		row.action->SetCenterPos({ keyLeft + kActionOffsetX + row.action->GetSize().x * kTextScale * 0.5f, y });

		m_rows.push_back(row);
	}
}

void ControlsGuide::Update()
{
	// ATBコマンドメニューを開いている間（メニューが右下に出る）と、プレイヤーが倒れた後は消す
	bool isShow = true;
	if (auto player = m_wpPlayer.lock())
	{
		isShow = !player->IsCommandMenuOpen() && !player->IsDead();
	}

	float target = isShow ? 1.0f : 0.0f;
	if (m_alpha < target)	m_alpha = std::min(m_alpha + kFadeSpeed, target);
	else					m_alpha = std::max(m_alpha - kFadeSpeed, target);
}

void ControlsGuide::DrawSprite()
{
	if (m_alpha <= 0.0f) return;

	// うしろの半透明の板
	float height = kPadding * 2.0f + kRowSpacing * static_cast<float>(kControls.size());
	int centerX = static_cast<int>(kRightX - kPanelWidth * 0.5f);
	int centerY = static_cast<int>(kBottomY + height * 0.5f);
	Math::Color panelColor = kPanelColor;
	panelColor.w *= m_alpha;
	KdShaderManager::Instance().m_spriteShader.DrawBox(centerX, centerY,
		static_cast<int>(kPanelWidth * 0.5f), static_cast<int>(height * 0.5f), &panelColor);

	// 文字
	for (const Row& row : m_rows)
	{
		Math::Color keyColor = kKeyColor;
		keyColor.w *= m_alpha;
		row.key->SetColor(keyColor);
		row.key->DrawSprite();

		Math::Color actionColor = kActionColor;
		actionColor.w *= m_alpha;
		row.action->SetColor(actionColor);
		row.action->DrawSprite();
	}
}
