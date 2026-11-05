#pragma once
#include "../Base/BaseGage.h"

class HPGage : public BaseGage
{
public:
	HPGage() {}
	~HPGage() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	void SetHpRatio(float ratio) { m_hpRatio = ratio; }

private:

	void Release();

	float m_hpRatio = 1.0f;
	int m_frame = 0;		// 点滅に使う、経過フレーム数
	int m_gageWidth = 200;
};