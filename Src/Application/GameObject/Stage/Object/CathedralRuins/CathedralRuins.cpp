#include "CathedralRuins.h"

void CathedralRuins::Init()
{
	m_spModel = std::make_shared<KdModelData>();
	m_spModel->Load("Asset/Models/GameObject/Stage/Object/CathedralRuins_01/CathedralRuins_01.gltf"); 

	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape
	(
		"CathedralRuins",
		m_spModel,
		KdCollider::TypeGround | KdCollider::TypeBump
	);

	Math::Matrix scale = Math::Matrix::CreateScale(0.25f);
	Math::Matrix rot = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(45.0f));
	Math::Matrix trans = Math::Matrix::CreateTranslation({ 0.0f, 0.0f, 60.0f });
	m_mWorld = scale * rot * trans;
}
