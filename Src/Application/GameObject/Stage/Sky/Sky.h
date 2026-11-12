#pragma once
#include"../Object/Base/BaseStageObject.h"

// 夜空と月
// ・夜空：大きな球の内側に、暗く晴れた夜空の画像（NightSky.png）を貼る。カメラに付いていく
// ・月　：空より少し手前に、月の画像（Moon.png・少し赤い）を貼った小さな球を置く。空と同じくカメラに付いていく
//         （付いていかないと、歩くと月に近づいてしまう）。まわりに、ごく薄い光のにじみ（MoonGlow.png）を加算で重ねる
// ・画像とモデルは Tools/MakeNightSky.py で作る
//
// 【スカイスフィア】カメラを包む大きな球の内側に空の画像を貼る。カメラに付いていくので、どこまで歩いても空は遠いまま
// 【なぜ DrawUnLit か】空と月は光の影響を受けない（受けると、月明かりの向きで空の明るさがむらになる）
// 【なぜ霧をかけないか】空は遠いので、かけると全部が霧の色になってしまう
// 【月の位置と大きさ】方角（yaw）と高さ（pitch）の角度から向きを作り、その向きに 120m 先に置く
//   見た目の大きさ（3.5 度）から、半径 ＝ 距離 × tan(角度 ÷ 2) で球の大きさを決める
//   距離は、空のドーム（半径 150m）より手前の 120m にする（遠くに置くと、先に描いた空に隠れて見えない）
//   月のモデルも空と同じ「下半分の無いドーム」なので、X 軸で 180 度回したものを下にもう 1 つ描いて、丸い球にする
// 【参考】スカイボックス・スカイスフィアの考え方（Real-Time Rendering など、3D グラフィックスの本）
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
	Math::Matrix						m_moonWorld;		// 月の上半分
	Math::Matrix						m_moonLowerWorld;	// 月の下半分（モデルがドームなので、上下さかさまにしてもう 1 つ描く）
	Math::Matrix						m_glowWorld;
};
