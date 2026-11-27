#pragma once
#include "../LOD.h"
#include <memory>

class KdModelData;
struct ID3D11DeviceContext;

namespace lod::kd
{
    // 1 メッシュ・1 段階のインデックスバッファと、材質ごとの描画範囲。
    // シェーダーは通常通り頂点・材質・ボーンを転送し、最後の DrawIndexed だけをここに任せる。
    struct MeshLevel
    {
        struct Storage;
        std::shared_ptr<const Storage> storage;
        void Bind(ID3D11DeviceContext* context) const;
        void DrawSubset(ID3D11DeviceContext* context, size_t subset) const;
    };

    struct SharedModel;

    // 所有単位は「画面に存在するキャラクター 1 体」。GPU データは同じモデル間で共有。
    class Instance
    {
    public:
        // Kd の GPU デバイス作成後、モデル読み込み直後に描画スレッドから呼ぶ。
        void Attach(const std::shared_ptr<KdModelData>& model, std::string_view path);
        // カメラが移動し終わったら 1 回だけ呼ぶ。負値はカメラ未設定として LOD0 に戻す。
        void SelectDistance(float distance, const Distances* overrideDistances = nullptr, int previewLevel = -1);
        const MeshLevel* ForNode(int nodeIndex) const;
        int Level() const { return m_level < 0 ? 0 : m_level; }
        float Distance() const { return m_distance; }
        size_t Triangles() const;
        bool Ready() const;

    private:
        std::shared_ptr<const SharedModel> m_shared;
        int m_level = -1;
        int m_autoLevel = -1; // 固定表示中も自動判定の履歴を保つ
        float m_distance = -1;
    };
}
