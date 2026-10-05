#pragma once

class TestModel : public KdGameObject
{
public:

	TestModel()		= default;
	~TestModel()	= default;

	void Init() override;
	void Update() override;
	void DrawLit() override;

private:

	std::shared_ptr<KdModelData> m_model = nullptr;
};