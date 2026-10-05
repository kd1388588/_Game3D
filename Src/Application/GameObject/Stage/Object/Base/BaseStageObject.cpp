#include "BaseStageObject.h"

void BaseStageObject::Init()
{
	m_spModel = nullptr;
}

void BaseStageObject::DrawLit()
{
	if (m_spModel)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
}

void BaseStageObject::Release()
{
	m_spModel = nullptr;
}
