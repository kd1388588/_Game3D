#pragma once
#include "../../../../../../Framework/GameObject/KdGameObject.h"

class BaseGage : public KdGameObject
{
public:
	BaseGage() {}
	~BaseGage() {}

	void Init() override;
	void Update() override;
	void DrawUnLit() override;
	void DrawSprite() override;

protected:

	void Release();

	KdTexture m_gageBase;
	KdTexture m_gage;

	Math::Matrix m_mWorld;
};