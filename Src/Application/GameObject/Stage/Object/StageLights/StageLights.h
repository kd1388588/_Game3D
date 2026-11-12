#pragma once

// ステージの光源（壁掛け松明・かがり火・燭台）
// ・光の位置は Tools/MakeCastleStage.py が書き出す RuinedRoadLights.csv から読む
// ・毎フレーム、カメラの近くの光だけを点光源として登録する（点光源は最大 100 個まで）
// ・炎の光（Flame.png）を、カメラの方へ向けて加算合成で描く。炎に合わせて明るさ・大きさをゆらす
//
// 【なぜ近くの光だけにするのか】点光源は、1 ピクセルごとに「全部の光」について明るさを計算する
//   数が多いほど重いので、カメラから 45m 以内の近い順に 60 個だけにしている
//   遠くの光は霧でかすむので、登録しなくても見た目はほとんど変わらない
// 【なぜ sin を 2 つ重ねるのか】sin 1 つだと、明るさが規則正しく波打って機械的に見える
//   速さの違う sin を重ねると、くり返しの周期が長くなり、炎のように不規則に見える
// 【ビルボード】炎の板を、いつもカメラの方へ向ける（Y 軸だけ回すので、上下はまっすぐ立ったまま）
// 【参考】点光源 https://learn.microsoft.com/ja-jp/windows/win32/direct3d9/light-types
//         加算合成 https://learn.microsoft.com/ja-jp/windows/win32/direct3d11/d3d10-graphics-programming-guide-blend-state
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
