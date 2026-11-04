#include "Sky.h"

namespace
{
	const std::string	kSkyModelPath	= "Asset/Models/GameObject/Stage/Sky/NightSky.gltf";
	const std::string	kMoonModelPath	= "Asset/Models/GameObject/Stage/Sky/Moon.gltf";
	const std::string	kMoonGlowPath	= "Asset/Models/GameObject/Stage/Sky/MoonGlow.png";

	// 空の球の大きさ（球のモデルの半径は 10 なので、半径は 10 × kScale = 300）
	constexpr float		kScale			= 15.0f;
	constexpr float		kHeight			= -1.0f;	// 球の中心の高さ

	// 月（控えめな大きさ。見た目の大きさ 約 3.5 度 ＝ 本物の月の 7 倍くらい）
	constexpr float		kModelRadius	= 10.0f;	// 球のモデルの半径
	constexpr float		kMoonDistance	= 250.0f;	// カメラからの距離（空の球より手前）
	constexpr float		kMoonAngleDeg	= 3.5f;		// 見た目の大きさ（度）
	constexpr float		kMoonYawDeg		= 5.0f;		// 見える方角（度。0 で奥（+Z）、プラスで右）
	constexpr float		kMoonPitchDeg	= 21.0f;	// 見える高さ（度。地平線から）
	constexpr float		kMoonSpinDeg	= 126.0f;	// 月の模様の向き（球を回して、暗い模様が見えるようにする）
	constexpr float		kGlowSizeRate	= 9.0f;		// にじみの大きさ（月の半径の何倍か）
	const Math::Color	kGlowColor		= { 0.8f, 0.8f, 0.8f, 1.0f };	// にじみの明るさ（画像の色に掛ける）
}

void Sky::Init()
{
	if (!m_spModel)
	{
		m_spModel = std::make_shared<KdModelData>();
		m_spModel->Load(kSkyModelPath);
	}
	if (!m_spMoonModel)
	{
		m_spMoonModel = std::make_shared<KdModelData>();
		m_spMoonModel->Load(kMoonModelPath);
	}
	if (!m_spMoonGlow)
	{
		m_spMoonGlow = std::make_shared<KdSquarePolygon>(kMoonGlowPath);
		m_spMoonGlow->SetPivot(KdSquarePolygon::PivotType::Center_Middle);
		m_spMoonGlow->SetScale(1.0f);
	}
}

void Sky::Update()
{
	// 空はカメラに付いていく（どこまで歩いても、空の端が見えないようにする）
	// 高さは固定にして、地平線の位置が上下に動かないようにする
	Math::Vector3 camPos = KdShaderManager::Instance().GetCameraCB().CamPos;
	m_mWorld = Math::Matrix::CreateScale(kScale)
		* Math::Matrix::CreateTranslation(camPos.x, kHeight, camPos.z);

	// 月：カメラから見て決まった方角・高さに置く
	float yaw = DirectX::XMConvertToRadians(kMoonYawDeg);
	float pitch = DirectX::XMConvertToRadians(kMoonPitchDeg);
	Math::Vector3 dir = { std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch) };
	Math::Vector3 moonPos = Math::Vector3(camPos.x, kHeight, camPos.z) + dir * kMoonDistance;
	float radius = kMoonDistance * std::tan(DirectX::XMConvertToRadians(kMoonAngleDeg * 0.5f));
	m_moonWorld = Math::Matrix::CreateScale(radius / kModelRadius)
		* Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(kMoonSpinDeg))
		* Math::Matrix::CreateTranslation(moonPos);

	// にじみ：月の位置で、カメラの方を向いた板
	Math::Matrix camMat = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	Math::Matrix face = camMat;
	face.Translation(Math::Vector3::Zero);
	float glowSize = radius * kGlowSizeRate;
	m_glowWorld = Math::Matrix::CreateScale(glowSize, glowSize, 1.0f) * face * Math::Matrix::CreateTranslation(moonPos);
}

void Sky::DrawUnLit()
{
	// 空と月にはフォグをかけない（遠くにあるので、かけると全体がフォグの色になる）
	auto& shader = KdShaderManager::Instance().m_StandardShader;
	if (m_spModel)
	{
		shader.SetFogEnable(false);
		shader.DrawModel(*m_spModel, m_mWorld);
	}
	if (m_spMoonModel)
	{
		shader.SetFogEnable(false);
		shader.DrawModel(*m_spMoonModel, m_moonWorld);
	}
}

void Sky::DrawEffect()
{
	if (!m_spMoonGlow) return;

	// 月のまわりのにじみ（加算・両面・深度を書かない。壁の向こうでは隠れる）
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	shaderManager.m_StandardShader.SetFogEnable(false);
	shaderManager.m_StandardShader.DrawPolygon(*m_spMoonGlow, m_glowWorld, kGlowColor);

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
