#pragma once

#include "../Base/BaseStageObject.h"

// 山の上の廃城（城の中の道・門楼・中庭・崩れた大聖堂・玉座の間。光源の小物の台も含む）
// 光そのもの（点光源・炎）は StageLights が出す
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
