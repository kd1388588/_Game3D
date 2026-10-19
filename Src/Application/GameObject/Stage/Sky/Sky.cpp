#include "Sky.h"

void Sky::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelData>();
		m_spModel->Load("Asset/Models/GameObject/Stage/Sky/Sky.gltf");
	}
}

namespace
{
	// 空の球の大きさと中心
	// ステージ（スタート地点 z=-12 ～ 大聖堂の奥 z=105）が全部中に入るように、ステージの真ん中を中心にして大きくする
	// （球のモデルの半径は 10 なので、半径は 10 × kScale = 250）
	constexpr float		kScale		= 25.0f;
	const Math::Vector3	kCenter		= { 0.0f, -1.0f, 45.0f };
}

void Sky::Update()
{
	m_mWorld = Math::Matrix::CreateScale(kScale)
		* Math::Matrix::CreateTranslation(kCenter);
}

void Sky::DrawUnLit()
{
	if (m_spModel)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
}