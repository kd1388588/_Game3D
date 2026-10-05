#pragma once
#include"../../../../Framework/Effekseer/KdEffekseerManager.h"

class BaseEffect : public KdEffekseerManager
{
public:
	BaseEffect() {}
	~BaseEffect() override {}
	void Init() override;
	void Update() override;
	void DrawUnLit()override;
	void SetEffect
	(
		const std::string& effectName,
		const Math::Vector3& pos,
		const float size = 1.0f,
		const float speed = 1.0f,
		const bool isLoop = false
	);

private:


};
