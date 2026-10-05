#pragma once
#include"../Object/Base/BaseStageObject.h"

class Sky : public BaseStageObject
{
public:
	Sky() {}
	~Sky()				override {}

	void Init()			override;
	void Update()		override;

	void DrawUnLit()	override;
};