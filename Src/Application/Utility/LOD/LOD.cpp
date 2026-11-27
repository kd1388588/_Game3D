#include "LOD.h"
#include "third_party/meshoptimizer/meshoptimizer.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace lod
{
    namespace
    {
        bool ValidRatios(const Ratios& r)
        {
            return r[1] > 0 && r[1] <= r[0] && r[0] <= 1;
        }
    }

    bool IsValid(const Distances& d)
    {
        return std::isfinite(d.firstSwitchDistance + d.switchInterval + d.switchMargin)
            && d.firstSwitchDistance > 0 && d.switchInterval > 0 && d.switchMargin >= 0
            && d.switchMargin < d.firstSwitchDistance && d.switchMargin * 2 < d.switchInterval;
    }

    bool IsValid(const Profile& p)
    {
        if (!IsValid(p.distances) || !ValidRatios(p.keep) ||
            !(p.maxError[0] >= 0 && p.maxError[0] <= p.maxError[1] && p.maxError[1] <= 1))
            return false;
        for (const auto& rule : p.materials)
            if (!ValidRatios(rule.keep)) return false;
        return true;
    }

    Ratios RatiosFor(const Profile& profile, std::string_view material)
    {
        for (const auto& rule : profile.materials)
            if (rule.name == material) return rule.keep;
        return profile.keep;
    }

    int SelectLevel(float distance, int previous, const Distances& d)
    {
        if (!std::isfinite(distance) || distance < 0 || !IsValid(d)) return 0;
        const float nearEdge = d.firstSwitchDistance;
        const float farEdge = nearEdge + d.switchInterval;
        if (previous < 0 || previous > 2)
            return int(distance >= nearEdge) + int(distance >= farEdge);

        // 現在の段階を維持する帯を設ける。境界上の停止中に交互に切り替わるのを防ぐ。
        const float edges[] = { nearEdge, farEdge };
        while (previous < 2 && distance >= edges[previous] + d.switchMargin) ++previous;
        while (previous > 0)
        {
            const float back = edges[previous - 1] - d.switchMargin;
            if (distance > back || (d.switchMargin == 0 && distance == back)) break;
            --previous;
        }
        return previous;
    }

    bool Simplify(const std::vector<Position>& positions, const std::vector<unsigned int>& indices,
                  float keep, float error, std::vector<unsigned int>& output)
    {
        output.clear();
        if (!(keep > 0 && keep <= 1 && error >= 0 && error <= 1) || indices.size() % 3 != 0)
            return false;
        if (indices.empty()) return true;
        if (positions.empty() || positions.size() > (std::numeric_limits<unsigned int>::max)()) return false;
        for (unsigned int i : indices)
        {
            if (i >= positions.size()) return false;
            const auto& p = positions[i];
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
        }

        // 小さなパーツにも最低 1 面を残す。材質を保護した時は順番も含めて元のまま返す。
        const size_t faces = indices.size() / 3;
        const size_t target = (std::max)(size_t(1), size_t(double(faces) * keep)) * 3;
        if (target == indices.size()) { output = indices; return true; }

        // meshoptimizer は元と同じ容量の書き込み先を要求する。戻り値が実際の個数。
        // LockBorder は別々に処理する材質の境界を保護する。元の頂点属性には書き込まない。
        output.resize(indices.size());
        const size_t count = meshopt_simplify(output.data(), indices.data(), indices.size(),
            &positions.front().x, positions.size(), sizeof(Position), target, error,
            meshopt_SimplifyLockBorder, nullptr);
        if (count == 0) output = indices;
        else output.resize(count);
        return true;
    }
}
