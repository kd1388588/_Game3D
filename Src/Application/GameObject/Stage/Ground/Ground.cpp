#include "Ground.h"

// ============================================================
// 【役割】ステージの下に敷く大きな地面（300 倍に広げる＝前後左右 ±300m）。歩ける地面として当たり判定を登録する
// 【なぜ 300 倍か】ステージ（約 x -55～58、z -18～135）より十分広くして、高く跳んで壁の外を見ても
//         地面が無い所（背景の青い色）が見えないようにする（Docs/CurrentState.txt の「ステージを作る時の注意」）
// 【元のテンプレートから変えた所】モデルが読めなかった時に、当たり判定を登録しないようにした
// 【なぜ KdModelWork か】元のテンプレートのまま。アニメーションしない物なので KdModelData でもよい
// ============================================================
void Ground::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelWork>();
		m_spModel->SetModelData("Asset/Models/GameObject/Stage/Ground/Ground.gltf");

		Math::Matrix scaleMat = Math::Matrix::CreateScale(300);
		Math::Matrix translationMat = Math::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
		m_mWorld = scaleMat * translationMat;

		// モデルが読み込めなかった場合は当たり判定を登録しない（空のモデルで判定すると落ちるため）
		if (m_spModel->IsEnable())
		{
			m_pCollider = std::make_unique<KdCollider>();
			m_pCollider->RegisterCollisionShape
			(
				"GroundCollision",
				m_spModel,
				KdCollider::TypeGround
			);
		}
	}
}

void Ground::DrawLit()
{
	if (!m_spModel) return;
	KdShaderManager::Instance().m_StandardShader.DrawModel(*m_spModel, m_mWorld);
}

void Ground::Release()
{
	m_spModel = nullptr;
}
