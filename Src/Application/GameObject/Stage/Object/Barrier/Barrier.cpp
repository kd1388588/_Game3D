#include "Barrier.h"

namespace
{
	const std::string	kTexturePath	= "Asset/Textures/Stage/Barrier.png";

	// 大聖堂の入口（Tools/MakeStageModel.py の NAVE_START_Z = 52、ENTRANCE_HALF_WIDTH = 9 に
	// ステージの拡大率 1.25 をかけた位置。両わきの柱に少しめり込ませてすき間をなくす）
	const Math::Vector3	kPosition		= { 0.0f, 0.0f, 65.0f };
	const Math::Vector2	kSize			= { 23.0f, 10.0f };		// 幅・高さ

	const Math::Vector3	kColor			= { 0.55f, 0.6f, 1.0f };	// 光の色（画像の色にかける）
	constexpr float		kPulseSpeed		= 0.05f;	// ゆらめきの速さ
	constexpr float		kPulseRange		= 0.25f;	// ゆらめきの強さ
	constexpr float		kFadeSpeed		= 0.02f;	// 開ける時に消える速さ（50フレームで消える）
}

void Barrier::Init()
{
	m_spPolygon = std::make_shared<KdSquarePolygon>(kTexturePath);
	m_spPolygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);
	m_spPolygon->SetScale(kSize);

	// 板で通れないようにする（ステージの壁と同じ TypeBump）
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Barrier", m_spPolygon, KdCollider::TypeBump);

	m_mWorld = Math::Matrix::CreateTranslation(kPosition);
}

void Barrier::Update()
{
	m_frame++;

	if (!m_isOpen) return;

	// 開けたら少しずつ消して、消えきったらオブジェクトを消す
	m_alpha -= kFadeSpeed;
	if (m_alpha <= 0.0f)
	{
		m_alpha = 0.0f;
		m_isExpired = true;
	}
}

void Barrier::Open()
{
	if (m_isOpen) return;
	m_isOpen = true;

	// 当たり判定はすぐに消す
	if (m_pCollider) m_pCollider->SetEnable("Barrier", false);
}

void Barrier::DrawEffect()
{
	if (!m_spPolygon || m_alpha <= 0.0f) return;

	// ゆらゆら明るさを変える
	float pulse = 1.0f + kPulseRange * std::sin(m_frame * kPulseSpeed);
	float bright = pulse * m_alpha;
	Math::Color color = { kColor.x * bright, kColor.y * bright, kColor.z * bright, 1.0f };

	// 光の膜なので、加算合成・両面・深度書き込みなしで描く
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	shaderManager.m_StandardShader.DrawPolygon(*m_spPolygon, m_mWorld, color);

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
