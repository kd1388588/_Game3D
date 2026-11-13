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
	float m_blinkPhase = 0.0f;	// 点滅の進み具合（sin に入れる角度。速さが変わっても明るさが飛ばない）
	int m_gageWidth = 200;
};