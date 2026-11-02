#pragma once

// ステージの光源（壁掛け松明・かがり火・燭台）
// ・光の位置は Tools/MakeStageModel.py が書き出す RuinedRoadLights.csv から読む
// ・毎フレーム、カメラの近くの光だけを点光源として登録する（点光源は最大 100 個まで）
// ・炎の光（Flame.png）を、カメラの方へ向けて加算合成で描く。炎に合わせて明るさ・大きさをゆらす
class StageLights : public KdGameObject
{
public:
	StageLights() {}
	~StageLights()override {}

	void Init()override;
	void Update()override;
	void DrawEffect()override;

private:

	struct Light
	{
		Math::Vector3	pos;
		Math::Vector3	color;
		float			radius = 1.0f;
		float			flameSize = 0.5f;
		float			phase = 0.0f;		// ゆらめきのずれ（光ごとにばらばらにする）
	};

	// その光の、今のゆらめきの強さ（1.0 前後）
	float CalcFlicker(const Light& light) const;

	std::vector<Light>					m_lights;
	std::vector<int>					m_activeIndices;	// 今のフレームに点光源として登録した光
	std::shared_ptr<KdSquarePolygon>	m_spFlame = nullptr;
	int									m_frame = 0;
};
