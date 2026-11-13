#pragma once

// 地面の霧：床の近くに、霧のかたまり（GroundMist.png）を散らして、ゆっくり流す
// ・かたまりはカメラのまわりにだけ出す。流れて範囲の外に出たら、反対側に出し直す（どこまで歩いても霧がある）
// ・濃さ・数・大きさ・流れなどの数値は、オブジェクトエディタ（StageSettings の GroundMist）で調整する
// ・フレームワークのシェーダーは、アルファが 0.55 より小さい所を描かない（アルファテスト）ので、
//   画像のアルファは 1 にして色の明るさで濃さを表し、加算合成で重ねる（Barrier・StageLights と同じ）
class GroundMist : public KdGameObject
{
public:
	GroundMist() {}
	~GroundMist()override {}

	void Init()override;
	void Update()override;
	void DrawEffect()override;

private:

	struct Puff
	{
		Math::Vector3	pos;				// 下の端の真ん中（y は床からの高さ）
		float			size = 1.0f;		// 幅
		float			bright = 1.0f;		// 濃さ（設定の濃さに掛ける）
		float			phase = 0.0f;		// ゆらぎのずれ
	};

	// カメラのまわり（cx, cz を中心に、半径 range の円の中）のどこかに置く
	void Respawn(Puff& puff, float cx, float cz, bool anywhere) const;

	std::vector<Puff>					m_puffs;
	std::shared_ptr<KdSquarePolygon>	m_spPolygon = nullptr;
	float								m_time = 0.0f;		// 経過時間（秒）
};
