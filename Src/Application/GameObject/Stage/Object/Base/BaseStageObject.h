#pragma once

class BaseStageObject : public KdGameObject
{
public:

	BaseStageObject() {}
	~BaseStageObject()override {}
	void Init()override;
	void DrawLit()override;

protected:

	void Release();

	std::shared_ptr<KdModelData> m_spModel = nullptr;

};