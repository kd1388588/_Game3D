#include "Ground.h"

void Ground::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/GameObject/Stage/Ground/Ground.gltf");

		Math::Matrix scaleMat = Math::Matrix::CreateScale(100);
		Math::Matrix translationMat = Math::Matrix::CreateTranslation(0.0f, -1.0f, 0.0f);
		m_mWorld = scaleMat * translationMat;

		// モデルが読み込めなかった場合は当たり判定を登録しない（空のモデルで判定すると落ちるため）
		if (m_spModel->IsEnable())
		{
			m_pCollider = std::make_unique<KdCollider>();
			m_pCollider->RegisterCollisionShape
			(
				"GroundCollision",
				m_spModel,
				KdCollider::TypeGround
			);
		}
	}
}

void Ground::DrawLit()
{
	if (!m_spModel) return;
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void Ground::Release()
{
	m_spModel = nullptr;
}
