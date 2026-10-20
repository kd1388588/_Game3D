#pragma once

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