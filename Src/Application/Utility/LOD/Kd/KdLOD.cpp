#include "Pch.h"
#include "KdLOD.h"
#include "../LODSettings.h"
#include <limits>
#include <cmath>

namespace lod::kd
{
    struct MeshLevel::Storage
    {
        struct Range { UINT first, count; }; // インデックス単位。1 三角形 = 3 インデックス
        Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        std::vector<Range> ranges;
    };

    struct SharedModel
    {
        // 元のメッシュと頂点番号の対応を保つため、LOD を使う間は元モデルも所有する。
        std::shared_ptr<KdModelData> source;
        const Profile* profile = nullptr;  // LODSettings.h の固定プロファイル
        std::array<size_t, 3> triangles{};
        std::array<std::vector<MeshLevel>, 2> nodes;
        bool ready = false;
    };

    void MeshLevel::Bind(ID3D11DeviceContext* context) const
    {
        context->IASetIndexBuffer(storage->buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    }

    void MeshLevel::DrawSubset(ID3D11DeviceContext* context, size_t subset) const
    {
        const auto& range = storage->ranges.at(subset);
        if (range.count) context->DrawIndexed(range.count, range.first, 0);
    }

    namespace
    {
        bool BuildMesh(const KdMesh& mesh, int node, SharedModel& model)
        {
            std::vector<Position> positions;
            positions.reserve(mesh.GetVertexPositions().size());
            for (const auto& p : mesh.GetVertexPositions()) positions.push_back({ p.x, p.y, p.z });

            // 材質は元の順序で保ち、1 メッシュにつき各段階でバッファを 1 本にまとめる。
            const auto& faces = mesh.GetFaces();
            const auto& materials = model.source->GetMaterials();
            std::array<std::vector<UINT>, 2> combined;
            std::array<std::vector<MeshLevel::Storage::Range>, 2> ranges;
            constexpr size_t maxIndices = (std::numeric_limits<UINT>::max)() / sizeof(UINT);
            for (const auto& subset : mesh.GetSubsets())
            {
                if (subset.FaceStart > faces.size() || subset.FaceCount > faces.size() - subset.FaceStart ||
                    subset.FaceCount > maxIndices / 3) return false;
                const bool hasMaterial = subset.MaterialNo < materials.size();
                if (!hasMaterial && subset.FaceCount) return false;
                const Ratios keep = hasMaterial ? RatiosFor(*model.profile, materials[subset.MaterialNo].m_name)
                                                : model.profile->keep;
                std::vector<UINT> original;
                original.reserve(size_t(subset.FaceCount) * 3);
                for (UINT f = 0; f < subset.FaceCount; ++f)
                {
                    const auto& face = faces[subset.FaceStart + f];
                    original.insert(original.end(), std::begin(face.Idx), std::end(face.Idx));
                }

                std::array<std::vector<UINT>, 2> reduced;
                for (size_t i = 0; i < reduced.size(); ++i)
                {
                    auto& indices = reduced[i];
                    if (!Simplify(positions, original, keep[i], model.profile->maxError[i], indices)) return false;
                    // 誤差上限で簡略化が止まっても、遠くの段階で面数が増えないようにする。
                    if (i && indices.size() > reduced[i - 1].size()) indices = reduced[i - 1];
                    auto& all = combined[i];
                    if (indices.size() > maxIndices - all.size()) return false;
                    ranges[i].push_back({ UINT(all.size()), UINT(indices.size()) });
                    all.insert(all.end(), indices.begin(), indices.end());
                }
            }

            for (size_t i = 0; i < combined.size(); ++i)
            {
                const auto& indices = combined[i];
                model.triangles[i + 1] += indices.size() / 3;
                // 全ての面が残ったメッシュは元の GPU バッファをそのまま使える。
                if (indices.empty() || indices.size() / 3 == faces.size()) continue;
                auto output = std::make_shared<MeshLevel::Storage>();
                output->ranges = std::move(ranges[i]);
                D3D11_BUFFER_DESC desc{};
                desc.ByteWidth = UINT(indices.size() * sizeof(UINT));
                desc.Usage = D3D11_USAGE_IMMUTABLE; // 生成後は毎フレーム読み取りのみ
                desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
                D3D11_SUBRESOURCE_DATA init{};
                init.pSysMem = indices.data();
                auto* device = KdDirect3D::Instance().WorkDev();
                if (!device || FAILED(device->CreateBuffer(&desc, &init, output->buffer.GetAddressOf()))) return false;
                model.nodes[i][node].storage = std::move(output);
            }
            return true;
        }

        std::shared_ptr<SharedModel> Build(const std::shared_ptr<KdModelData>& source, const Profile* profile)
        {
            auto model = std::make_shared<SharedModel>();
            model->source = source;
            model->profile = profile;
            const auto& drawNodes = source->GetDrawMeshNodeIndices();
            for (int n : drawNodes)
                if (auto mesh = source->GetMesh(n))
                    for (const auto& subset : mesh->GetSubsets()) model->triangles[0] += subset.FaceCount;

            if (!profile) return model; // このモデルには LOD が設定されていない
            if (!IsValid(*profile))
            {
                OutputDebugStringA("[LOD] Invalid profile. Using original mesh.\n");
                return model;
            }
            for (auto& level : model->nodes) level.resize(source->GetOriginalNodes().size());
            for (int n : drawNodes)
            {
                const auto mesh = source->GetMesh(n);
                if (mesh && !BuildMesh(*mesh, n, *model))
                {
                    // 一部だけ出来た GPU リソースは解放し、本体・影ともに LOD0 に戻す。
                    for (auto& level : model->nodes) level.clear();
                    OutputDebugStringA("[LOD] Could not create index buffers. Using original mesh.\n");
                    return model;
                }
            }
            model->ready = true;
            return model;
        }
    }

    void Instance::Attach(const std::shared_ptr<KdModelData>& source, std::string_view path)
    {
        m_level = m_autoLevel = -1;
        m_distance = -1;
        if (!source) { m_shared.reset(); return; }
        const auto* profile = settings::Find(path);
        // 読み込みは Kd の描画スレッドで行う。最後の所有者が消えたら GPU メモリが解放される。
        static std::vector<std::weak_ptr<const SharedModel>> cache;
        cache.erase(std::remove_if(cache.begin(), cache.end(), [](const auto& e) { return e.expired(); }), cache.end());
        for (const auto& entry : cache)
        {
            auto shared = entry.lock();
            if (shared && shared->source == source && shared->profile == profile)
            {
                m_shared = std::move(shared);
                return;
            }
        }
        m_shared = Build(source, profile);
        cache.emplace_back(m_shared);
    }

    void Instance::SelectDistance(float distance, const Distances* overrideDistances, int previewLevel)
    {
        m_distance = distance;
        if (!Ready() || !std::isfinite(distance) || distance < 0) { m_level = m_autoLevel = -1; return; }
        const auto& distances = overrideDistances ? *overrideDistances : m_shared->profile->distances;
        m_autoLevel = SelectLevel(distance, m_autoLevel, distances);
        m_level = previewLevel >= 0 && previewLevel <= 2 ? previewLevel : m_autoLevel;
    }

    const MeshLevel* Instance::ForNode(int node) const
    {
        if (Level() == 0) return nullptr;
        const auto& nodes = m_shared->nodes[Level() - 1];
        if (node < 0 || size_t(node) >= nodes.size() || !nodes[node].storage) return nullptr;
        return &nodes[node];
    }

    size_t Instance::Triangles() const { return m_shared ? m_shared->triangles[Level()] : 0; }
    bool Instance::Ready() const { return m_shared && m_shared->ready; }
}
