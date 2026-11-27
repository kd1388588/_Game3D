#pragma once
#include "LOD.h"

namespace lod::settings
{
    // 【調整場所】モデルを追加する場合は profiles に 1 項目追加し、読み込み側で Attach する。
    // パスはモデル読み込み時と同じ表記。ゲームの通常敵・中ボス・ボスはこのモデルを共有する。
    inline constexpr Distances enemyDistances{
        12.0f, // firstSwitchDistance: 遠くするほど元の姿を長く表示
        14.0f, // switchInterval: 2 番目の境界 = 12 + 14 = 26
         1.0f  // switchMargin: 離れると 13 / 27、戻ると 11 / 25
    };
    inline const Profile profiles[] = {
        {
            "Asset/Models/GameObject/Enemy/Monster2/Base/Rampage.gltf",
            enemyDistances,
            { 0.50f, 0.25f }, // keep: LOD1 / LOD2 で残す割合。0.50 は「半分残す」
            { 0.015f, 0.035f }, // maxError: 小さくすると輪郭を守り、面が多く残る
            {
                // 小さな目元は両段階で元の三角形を使用。他の材質は上の keep を使う。
                { "M_Eye_Occlusion", { 1.0f, 1.0f } },
                // 鎧だけ強く減らす例。先頭の // を外すと有効。
                // { "M_Rampage_Armor_Rampage", { 0.40f, 0.20f } },
            }
        },
    };

    inline const Profile* Find(std::string_view path)
    {
        for (const auto& profile : profiles)
            if (profile.modelPath == path) return &profile;
        return nullptr; // 登録していないモデルは通常描画
    }
}
