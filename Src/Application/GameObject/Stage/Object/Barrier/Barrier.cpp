#include "Barrier.h"

namespace
{
	const std::string	kTexturePath	= "Asset/Textures/Stage/Barrier.png";

	constexpr float		kPulseSpeed		= 0.05f;	// ゆらめきの速さ
	constexpr float		kPulseRange		= 0.25f;	// ゆらめきの強さ
	constexpr float		kFadeSpeed		= 0.02f;	// 開ける時に消える速さ（50フレームで消える）
}

void Barrier::Init()
{
	m_spPolygon = std::make_shared<KdSquarePolygon>(kTexturePath);
	m_spPolygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);
	m_spPolygon->SetScale(m_size);

	// 板で通れないようにする（ステージの壁と同じ TypeBump）
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape("Barrier", m_spPolygon, KdCollider::TypeBump);

	// 板の向き（当たり判定も、この行列で同じように回る）
	m_mWorld = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_yawDeg)) * Math::Matrix::CreateTranslation(m_pos);
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
	Math::Color color = { m_color.x * bright, m_color.y * bright, m_color.z * bright, 1.0f };

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
