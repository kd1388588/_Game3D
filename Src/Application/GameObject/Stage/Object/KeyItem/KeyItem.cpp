#include "KeyItem.h"
#include "../../../Character/Player/Player.h"

namespace
{
	const std::string	kTexturePath	= "Asset/Textures/Stage/Key.png";	// Tools/MakeKeyTexture.py で作成

	constexpr float		kSize			= 1.2f;		// 鍵の板の大きさ（m）
	constexpr float		kBobHeight		= 0.15f;	// 上下に動く幅（m）
	constexpr float		kBobSpeed		= 0.05f;	// 上下に動く速さ
	constexpr float		kAppearSpeed	= 0.03f;	// 出てくる速さ（約 0.5 秒で大きくなる）
	constexpr float		kPickRange		= 1.8f;		// これより近づくと拾う（横の距離。m）
	constexpr float		kPickHeight		= 3.0f;		// 高さの差がこれより小さい時だけ拾う（上を飛び越えた時に拾わないように）

	// 光（金色の点光源。まわりの床や柱を照らして、遠くからでも場所が分かるように）
	const Math::Vector3	kLightColor		= { 1.0f, 0.75f, 0.3f };
	constexpr float		kLightIntensity	= 3.0f;
	constexpr float		kLightRadius	= 6.0f;
}

void KeyItem::Init()
{
	m_spPolygon = std::make_shared<KdSquarePolygon>(kTexturePath);
	m_spPolygon->SetPivot(KdSquarePolygon::PivotType::Center_Middle);
	m_spPolygon->SetScale(1.0f);
}

void KeyItem::Update()
{
	m_frame++;
	m_appearRate = std::min(m_appearRate + kAppearSpeed, 1.0f);

	// 金色の光（点光源はフレームの最初に空になるので、毎フレーム登録する）
	float pulse = 0.85f + 0.15f * std::sin(m_frame * kBobSpeed * 2.0f);
	KdShaderManager::Instance().WorkAmbientController().AddPointLight(
		kLightColor * (kLightIntensity * pulse * m_appearRate), kLightRadius, m_basePos, true);

	if (m_isPicked) return;

	// プレイヤーが近づいたら拾う（出てきている途中でも拾える）
	auto player = m_wpPlayer.lock();
	if (!player || !player->IsAlive()) return;

	Math::Vector3 diff = player->GetPos() - m_basePos;
	float height = std::abs(diff.y);
	diff.y = 0.0f;
	if (diff.LengthSquared() <= kPickRange * kPickRange && height <= kPickHeight)
	{
		m_isPicked = true;
		m_isExpired = true;		// 次のフレームの最初に、シーンから消える
	}
}

void KeyItem::DrawEffect()
{
	if (!m_spPolygon || m_isPicked) return;

	// 上下にふわふわ動かす
	Math::Vector3 pos = m_basePos + Math::Vector3(0.0f, std::sin(m_frame * kBobSpeed) * kBobHeight, 0.0f);

	// カメラの方を向けた板（ビルボード）：カメラの行列（ビューの逆）の回転だけを使う
	Math::Matrix face = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	face.Translation(Math::Vector3::Zero);

	float size = kSize * m_appearRate;
	Math::Matrix mWorld = Math::Matrix::CreateScale(size, size, 1.0f) * face * Math::Matrix::CreateTranslation(pos);

	// 光る物なので、加算合成・両面・深度を書かないで描く（画像の黒い所は見えない）
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	const Math::Color color = { 1.0f, 1.0f, 1.0f, 1.0f };
	shaderManager.m_StandardShader.DrawPolygon(*m_spPolygon, mWorld, color);

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
