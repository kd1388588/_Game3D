#include "TitleEmber.h"
#include "../../../Utility/SpriteUtil.h"

namespace
{
	constexpr int	kGlowTexSize		= 64;

	// 火の粉
	constexpr int	kEmberNum			= 70;
	constexpr float	kEmberMinSize		= 6.0f;
	constexpr float	kEmberMaxSize		= 18.0f;
	constexpr float	kEmberMinSpeed		= 0.6f;		// 上昇の速さ（ピクセル/フレーム）
	constexpr float	kEmberMaxSpeed		= 1.8f;
	constexpr int	kEmberMinLife		= 240;
	constexpr int	kEmberMaxLife		= 600;
	constexpr int	kEmberBlueRate		= 25;		// 青い粒（剣の軌跡と同じ色）にする割合（％）
	const Math::Color kEmberColor		= { 1.0f, 0.55f, 0.2f, 1.0f };
	const Math::Color kEmberBlueColor	= { 0.3f, 0.7f, 1.0f, 1.0f };

	// 光の玉
	constexpr int	kBokehNum			= 10;
	constexpr float	kBokehMinSize		= 120.0f;
	constexpr float	kBokehMaxSize		= 300.0f;
	constexpr float	kBokehMaxSpeed		= 0.25f;
	constexpr int	kBokehMinLife		= 600;
	constexpr int	kBokehMaxLife		= 1200;
	constexpr float	kBokehBrightness	= 0.12f;	// 光の玉は背景を邪魔しないよう薄くする

	constexpr int	kFadeFrame			= 60;		// 出てくる時・消える時にかけるフレーム数
	constexpr float	kSwaySpeed			= 0.02f;	// 左右に揺れる速さ
	constexpr float	kSpawnMargin		= 40.0f;	// 画面の外のどこから出すか
}

void TitleEmber::Init()
{
	m_glowTex = SpriteUtil::CreateGlowTexture(kGlowTexSize);
	m_screenSize = SpriteUtil::GetScreenSize();

	m_embers.resize(kEmberNum);
	for (auto& p : m_embers) { SpawnEmber(p, true); }

	m_bokehs.resize(kBokehNum);
	for (auto& p : m_bokehs) { SpawnBokeh(p, true); }
}

void TitleEmber::SpawnEmber(Particle& p, bool isFirst)
{
	float halfW = m_screenSize.x * 0.5f;
	float halfH = m_screenSize.y * 0.5f;

	p.pos.x = KdRandom::GetFloat(-halfW, halfW);
	p.pos.y = isFirst ? KdRandom::GetFloat(-halfH, halfH) : -halfH - kSpawnMargin;
	p.velocity = { KdRandom::GetFloat(-0.3f, 0.3f), KdRandom::GetFloat(kEmberMinSpeed, kEmberMaxSpeed) };
	p.color = (KdRandom::GetInt(0, 99) < kEmberBlueRate) ? kEmberBlueColor : kEmberColor;
	p.size = KdRandom::GetFloat(kEmberMinSize, kEmberMaxSize);
	p.swayPhase = KdRandom::GetFloat(0.0f, DirectX::XM_2PI);
	p.swayWidth = KdRandom::GetFloat(0.2f, 0.8f);
	p.maxLife = KdRandom::GetInt(kEmberMinLife, kEmberMaxLife);
	p.life = isFirst ? KdRandom::GetInt(kFadeFrame, p.maxLife) : p.maxLife;
}

void TitleEmber::SpawnBokeh(Particle& p, bool isFirst)
{
	float halfW = m_screenSize.x * 0.5f;
	float halfH = m_screenSize.y * 0.5f;

	p.pos = { KdRandom::GetFloat(-halfW, halfW), KdRandom::GetFloat(-halfH, halfH) };
	p.velocity = { KdRandom::GetFloat(-kBokehMaxSpeed, kBokehMaxSpeed), KdRandom::GetFloat(0.05f, kBokehMaxSpeed) };
	p.color = (KdRandom::GetInt(0, 1) == 0) ? kEmberColor : kEmberBlueColor;
	p.size = KdRandom::GetFloat(kBokehMinSize, kBokehMaxSize);
	p.swayPhase = KdRandom::GetFloat(0.0f, DirectX::XM_2PI);
	p.swayWidth = 0.1f;
	p.maxLife = KdRandom::GetInt(kBokehMinLife, kBokehMaxLife);
	p.life = isFirst ? KdRandom::GetInt(kFadeFrame, p.maxLife) : p.maxLife;
}

bool TitleEmber::UpdateParticle(Particle& p, int frame)
{
	// 上昇しながら、ゆらゆらと左右に揺れる
	p.pos += p.velocity;
	p.pos.x += std::sin(frame * kSwaySpeed + p.swayPhase) * p.swayWidth;

	p.life--;
	return p.life > 0;
}

float TitleEmber::CalcFade(const Particle& p)
{
	float fadeIn = std::clamp(static_cast<float>(p.maxLife - p.life) / kFadeFrame, 0.0f, 1.0f);
	float fadeOut = std::clamp(static_cast<float>(p.life) / kFadeFrame, 0.0f, 1.0f);
	return fadeIn * fadeOut;
}

void TitleEmber::Update()
{
	m_frame++;

	float topY = m_screenSize.y * 0.5f + kSpawnMargin;

	for (auto& p : m_embers)
	{
		// 寿命が尽きた・画面の上に出た粒は、画面の下から出し直す
		if (!UpdateParticle(p, m_frame) || p.pos.y > topY)
		{
			SpawnEmber(p, false);
		}
	}

	for (auto& p : m_bokehs)
	{
		if (!UpdateParticle(p, m_frame))
		{
			SpawnBokeh(p, false);
		}
	}
}

void TitleEmber::DrawParticles(const std::vector<Particle>& particles, float brightness) const
{
	auto& spriteShader = KdShaderManager::Instance().m_spriteShader;

	for (const auto& p : particles)
	{
		// 火の粉がちらちら瞬くように、明るさを細かく揺らす
		float flicker = 0.75f + 0.25f * std::sin(m_frame * 0.2f + p.swayPhase * 7.0f);

		Math::Color color = p.color;
		color.w = CalcFade(p) * flicker * brightness;

		int size = static_cast<int>(p.size);
		spriteShader.DrawTex(m_glowTex.get(), static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), size, size, nullptr, &color);
	}
}

void TitleEmber::DrawSprite()
{
	if (!m_glowTex) return;

	// 光を重ねて明るくするため加算合成で描く
	KdShaderManager::Instance().ChangeBlendState(KdBlendState::Add);

	DrawParticles(m_bokehs, kBokehBrightness);
	DrawParticles(m_embers, 1.0f);

	KdShaderManager::Instance().UndoBlendState();
}
