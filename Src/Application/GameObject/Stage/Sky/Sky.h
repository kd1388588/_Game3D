#pragma once
#include"../Object/Base/BaseStageObject.h"

// 夜空と月
// ・夜空：大きな球の内側に、暗く晴れた夜空の画像（NightSky.png）を貼る。カメラに付いていく
// ・月　：空より少し手前に、月の画像（Moon.png・少し赤い）を貼った小さな球を置く。空と同じくカメラに付いていく
//         （付いていかないと、歩くと月に近づいてしまう）。まわりに、ごく薄い光のにじみ（MoonGlow.png）を加算で重ねる
// ・画像とモデルは Tools/MakeNightSky.py で作る
class Sky : public BaseStageObject
{
public:
	Sky() {}
	~Sky()				override {}

	void Init()			override;
	void Update()		override;

	void DrawUnLit()	override;
	void DrawEffect()	override;

private:

	std::shared_ptr<KdModelData>		m_spMoonModel = nullptr;
	std::shared_ptr<KdSquarePolygon>	m_spMoonGlow = nullptr;
	Math::Matrix						m_moonWorld;
	Math::Matrix						m_glowWorld;
};
