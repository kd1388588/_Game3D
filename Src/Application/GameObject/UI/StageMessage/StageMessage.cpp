#include "StageMessage.h"
#include "../Text/UIText.h"

namespace
{
	// メッセージ（画面の真ん中より少し上）
	const Math::Vector2	kMessagePos		= { 0.0f, 150.0f };
	constexpr float		kMessageScale	= 0.8f;					// Medium（40px）× 0.8
	const Math::Color	kMessageColor	= { 1.0f, 0.9f, 0.65f, 1.0f };	// 少し金色がかった白
	constexpr int		kFadeInFrame	= 12;					// 出てくる時間
	constexpr int		kFadeOutFrame	= 40;					// 消える時間

	// 鍵を持っている表示（左下の HUD の、モードの文字の上）
	const std::string	kKeyTexPath = "Asset/Textures/Stage/Key.png";
	const Math::Vector2	kKeyPos = { -480.0f, -235.0f };			// 画面の真ん中が (0, 0)
	constexpr int		kKeySize = 48;							// 描く大きさ（元の画像の大きさとは別）
	const Math::Color	kKeyColor = { 1.0f, 1.0f, 1.0f, 1.0f };	// 白：画像の色のまま
}

void StageMessage::Init()
{
	m_text = std::make_shared<UIText>();
	m_text->SetCenterPos(kMessagePos);
	m_text->SetScale(kMessageScale);

	m_KeyTex = std::make_shared<KdTexture>();
	if (!m_KeyTex->Load(kKeyTexPath))
	{
		// 読めなかった時は空にしておく（描く時に、空なら何もしない）
		OutputDebugStringA(("【鍵の画像が読み込めませんでした】" + kKeyTexPath + "\n").c_str());
		m_KeyTex = nullptr;
	}
}

void StageMessage::Show(const std::string& text, int frames)
{
	if (!m_text) return;
	m_text->SetText(text, UIText::FontSize::Medium);
	m_timer = frames;
	m_maxTimer = frames;
}

void StageMessage::Update()
{
	if (m_timer > 0) m_timer--;
}

void StageMessage::DrawSprite()
{
	// メッセージ：出てくる時と消える時に、不透明度を変える
	if (m_text && m_timer > 0)
	{
		int elapsed = m_maxTimer - m_timer;
		float alpha = 1.0f;
		if (elapsed < kFadeInFrame)		alpha = static_cast<float>(elapsed) / kFadeInFrame;
		if (m_timer < kFadeOutFrame)	alpha = std::min(alpha, static_cast<float>(m_timer) / kFadeOutFrame);

		Math::Color color = kMessageColor;
		color.w = alpha;
		m_text->SetColor(color);
		m_text->DrawSprite();
	}

	// 鍵を持っている間の表示
	if (m_KeyTex && m_hasKey)
	{
		KdShaderManager::Instance().m_spriteShader.DrawTex(m_KeyTex.get(),
			static_cast<int>(kKeyPos.x), static_cast<int>(kKeyPos.y),
			kKeySize, kKeySize, nullptr, &kKeyColor);
	}
}
