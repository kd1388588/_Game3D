#include "TestModel.h"

// プレイヤー
#include <Application/GameObject/Character/Player/Player.h>

void TestModel::Init()
{
	//m_model = std::make_shared<KdModelWork>();

	//// C:\3DGame_Braver\N_3DGame\Asset\Models\GameObject\TestModel
	//m_model->SetModelData("Asset/Models/GameObject/TestModel/Mutanto.gltf");

	m_model = KdAssets::Instance().m_modeldatas.GetData("Asset/Models/GameObject/Enemy/Player/Player.gltf");

	Math::Matrix scaleMat = Math::Matrix::CreateScale(0.1f);
	Math::Matrix translationMat = Math::Matrix::CreateTranslation(0.0f, 5.0f, 0.0f);
	m_mWorld = scaleMat * translationMat;
}

void TestModel::Update()
{

}

void TestModel::DrawLit()
{
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_model, m_mWorld);
}
