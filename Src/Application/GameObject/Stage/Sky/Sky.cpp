#include "Sky.h"

void Sky::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelData>();
		m_spModel->Load("Asset/Models/GameObject/Stage/Sky/Sky.gltf");
	}
}

void Sky::Update()
{
	m_mWorld = Math::Matrix::CreateScale(10.0f);
}

void Sky::DrawUnLit()
{
	if (m_spModel)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
}