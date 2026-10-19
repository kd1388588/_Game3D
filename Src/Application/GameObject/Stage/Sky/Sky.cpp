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
	// 空の球の大きさ（球のモデルの半径は 10 なので、半径は 10 × kScale = 300）
	constexpr float		kScale		= 15.0f;
	constexpr float		kHeight		= -1.0f;	// 球の中心の高さ
}

void Sky::Update()
{
	// 空はカメラに付いていく（どこまで歩いても、空の端が見えないようにする）
	// 高さは固定にして、地平線の位置が上下に動かないようにする
	Math::Vector3 camPos = KdShaderManager::Instance().GetCameraCB().CamPos;
	m_mWorld = Math::Matrix::CreateScale(kScale)
		* Math::Matrix::CreateTranslation(camPos.x, kHeight, camPos.z);
}

void Sky::DrawUnLit()
{
	if (m_spModel)
	{
		KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
	}
}