#pragma once

class Ground : public KdGameObject
{
public:

	Ground() {}
	~Ground()override {}

	void Init()override;
	void DrawLit()override;

private:

	void Release();

	std::shared_ptr<KdModelWork> m_spModel = nullptr;

};