#pragma once
#include "../../../../../../Framework/GameObject/KdGameObject.h"

class BaseGage : public KdGameObject
{
public:
	BaseGage() {}
	~BaseGage() override {}

	void Init() override;
	void Update() override;
	void DrawUnLit() override;
	void DrawSprite() override;

protected:

	void Release();

	KdTexture m_gageBase;
	KdTexture m_gage;
};