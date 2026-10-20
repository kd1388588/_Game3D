#pragma once

#include "../Base/BaseStageObject.h"

// 廃墟の参道と崩れた大聖堂（石畳の道・折れた柱・崩れた壁・門・ボスと戦う広間）
// モデルは Tools/MakeStageModel.py で作成
class RuinedRoad : public BaseStageObject
{
public:
	RuinedRoad() {}
	~RuinedRoad()override {}

	void Init()override;

private:

	// 柱のまわりの見えない当たり判定のモデル（描画しない）
	std::shared_ptr<KdModelData>	m_spGuardModel = nullptr;
};
