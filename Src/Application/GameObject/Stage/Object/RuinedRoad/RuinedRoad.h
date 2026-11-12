#pragma once

#include "../Base/BaseStageObject.h"

// 夜の廃墟の城（6 つの部屋を通路でつないだ形。光源の小物の台も含む）
// 光そのもの（点光源・炎）は StageLights が出す
// モデルは Tools/MakeCastleStage.py で作成（部屋の並びの図は、そのファイルの上の方と Docs/StageMap.png）
// ※クラス名とファイル名は、前のステージ（廃墟の参道）の時のまま使っている
//
// 【当たり判定の分け方】
//   ・見た目のモデルを、そのまま地面（TypeGround）と壁（TypeBump）の両方に使う
//   ・柱のまわりには、見えない単純な形（RuinedRoadCollision.gltf）を、壁（TypeBump）だけで足す
//     → 柱の細かい飾りに体が引っかからず、上に乗ることもできない
// 【弱点】見た目のモデルの三角形をそのまま当たり判定に使っているので、三角形が多いほど判定が重い
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
