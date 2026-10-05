#pragma once
#include "../Base/BaseGage.h"

class HPGage : public BaseGage
{
public:
	HPGage() {}
	~HPGage() {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	void SetHpRatio(float ratio) { m_hpRatio = ratio; }

private:

	void Release();

	float m_hpRatio = 1.0f;

	Math::Matrix m_mWorld = Math::Matrix::Identity;
	int m_gageWidth = 200;
};