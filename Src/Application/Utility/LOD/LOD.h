#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

namespace lod
{
    // LOD0 は元モデルそのもの。配列の [0] が LOD1、[1] が LOD2 に対応する。
    using Ratios = std::array<float, 2>;

    struct Distances
    {
        float firstSwitchDistance = 12.0f;  // 最初に軽くする距離（ワールド単位）
        float switchInterval = 14.0f;       // 1 番目と 2 番目の境界の間隔
        float switchMargin = 1.0f;          // 各境界の片側の遊び。0 なら即時切り替え
    };

    struct MaterialRule
    {
        std::string_view name;  // 読み込んだモデルの材質名に完全一致
        Ratios keep;
    };

    struct Profile
    {
        std::string_view modelPath;
        Distances distances;
        Ratios keep;                   // 元の三角形数に対する残存率。1 はそのまま使う
        Ratios maxError;               // meshoptimizer に渡す相対形状誤差の上限
        std::vector<MaterialRule> materials;
    };

    // ゲーム・描画 API に依存しない読み込み用の座標。元の GPU 頂点番号と並びをそろえる。
    struct Position { float x, y, z; };

    bool IsValid(const Profile& profile);
    bool IsValid(const Distances& settings);
    Ratios RatiosFor(const Profile& profile, std::string_view material);

    // previous = -1 は初回を表す。ワープ時は 1 フレームで 2 段階先へ移動できる。
    int SelectLevel(float distance, int previous, const Distances& settings);

    // 三角形リストの 1 材質分を簡略化。結果は元の頂点を指すインデックス。
    // 不正な入力なら false を返し、output を空にする（描画側で元モデルを使える）。
    bool Simplify(const std::vector<Position>& positions, const std::vector<unsigned int>& indices,
                  float keepRatio, float maxError, std::vector<unsigned int>& output);
}
