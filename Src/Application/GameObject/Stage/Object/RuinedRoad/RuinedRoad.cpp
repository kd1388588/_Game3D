#include "RuinedRoad.h"

namespace
{
	const std::string	kModelPath	= "Asset/Models/GameObject/Stage/RuinedRoad/RuinedRoad.gltf";

	// 置く位置（石畳の上の面が y = 0.04 になるように作ってある。地面に埋まる・浮く時はここで調整）
	const Math::Vector3	kPosition	= { 0.0f, 0.0f, 0.0f };
	const Math::Vector3	kScale		= { 1.25f, 1.25f, 1.25f };
}

void RuinedRoad::Init()
{
	m_spModel = std::make_shared<KdModelData>();
	if (!m_spModel->Load(kModelPath))
	{
		// モデルが無い時は何も出さない（空のモデルで当たり判定を作ると落ちるため）
		OutputDebugStringA(("【ステージのモデルが読み込めませんでした】" + kModelPath + "\n").c_str());
		m_spModel = nullptr;
		return;
	}

	// 石畳の上は歩けて、柱・壁・瓦礫は通り抜けられない
	m_pCollider = std::make_unique<KdCollider>();
	m_pCollider->RegisterCollisionShape
	(
		"RuinedRoad",
		m_spModel,
		KdCollider::TypeGround | KdCollider::TypeBump
	);

	m_mWorld = Math::Matrix::CreateScale(kScale) * Math::Matrix::CreateTranslation(kPosition);
}
