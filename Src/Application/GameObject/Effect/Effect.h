#pragma once
#include "../../../Framework/GameObject/KdGameObject.h"
#include "../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

class Effect : public KdGameObject
{
public:
	Effect() {}
	~Effect() override {}

	void Init() override;
	void Update() override;
	void DrawUnLit() override;

	// エフェクトを再生・セットする関数
	void SetEffect
	(
		const std::string& effectName,
		const Math::Vector3& pos,
		const float size = 1.0f,
		const float speed = 1.0f,
		const bool isLoop = false
	);

	void SetWorldMatrix(const Math::Matrix& m)
	{
		if (m_effect)
		{
			m_effect->SetWorldMatrix(m);
		}
	}

protected:
	// 派生クラス（Effect等）からも触れるように protected にしておく
	std::shared_ptr<KdEffekseerObject>	m_effect = nullptr;
};