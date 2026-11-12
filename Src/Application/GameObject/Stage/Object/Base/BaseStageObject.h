#pragma once

// ============================================================
// 【役割】ステージの物（建物など）の親クラス
//         モデル（KdModelData）を 1 つ持ち、光の当たる描画（DrawLit）と、影（GenerateDepthMapFromLight）をする
// 【子のクラス】RuinedRoad（今のステージ）、Sky（空と月）、CathedralRuins（今は使っていない）
// 【元のテンプレートから変えた所】影を落とすように GenerateDepthMapFromLight を足した
// ============================================================
class BaseStageObject : public KdGameObject
{
public:

	BaseStageObject() {}
	~BaseStageObject()override {}
	void Init()override;
	void DrawLit()override;
	void GenerateDepthMapFromLight()override;	// 影（柱や壁の影を落とす）

protected:

	void Release();

	std::shared_ptr<KdModelData> m_spModel = nullptr;

};