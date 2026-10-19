#pragma once

#include "../Base/BaseStageObject.h"

// 廃墟の参道（スタート地点から大聖堂の廃墟へ向かう石畳の道・折れた柱・崩れた壁・門）
// モデルは Tools/MakeStageModel.py で作成
class RuinedRoad : public BaseStageObject
{
public:
	RuinedRoad() {}
	~RuinedRoad()override {}

	void Init()override;
};
