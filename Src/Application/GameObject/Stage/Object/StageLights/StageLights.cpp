#include "StageLights.h"

namespace
{
	const std::string	kLightsPath		= "Asset/Models/GameObject/Stage/RuinedRoad/RuinedRoadLights.csv";
	const std::string	kFlameTexPath	= "Asset/Textures/Stage/Flame.png";	// Tools/MakeFlameTexture.py で作成

	// ステージのモデルと同じ拡大率（RuinedRoad.cpp の kScale と同じにする）
	constexpr float		kStageScale		= 1.25f;

	// 点光源として登録する光
	constexpr int		kMaxActiveLights	= 60;		// 一度に登録する数（シェーダーの上限は 100）
	constexpr float		kActiveRange		= 45.0f;	// カメラからこの距離までの光だけを登録する
	constexpr float		kLightIntensity		= 3.0f;		// 光の色に掛ける明るさ
	constexpr float		kLightRadiusRate	= 1.3f;		// 光の届く半径に掛ける倍率（松明のまわりだけでなく、少し離れた所も照らす）

	// 炎のゆらめき
	constexpr float		kFlickerSpeed1		= 0.21f;
	constexpr float		kFlickerSpeed2		= 0.53f;
	constexpr float		kFlickerRange		= 0.12f;	// 明るさのゆれ幅（1.0 ± これくらい）
	constexpr float		kFlameAspect		= 2.0f;		// 炎の画像の 高さ ÷ 幅
	constexpr float		kFlameBrightness	= 1.4f;		// 炎の光の明るさ
}

void StageLights::Init()
{
	m_spFlame = std::make_shared<KdSquarePolygon>(kFlameTexPath);
	m_spFlame->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);
	m_spFlame->SetScale(1.0f);

	// 光の位置を読む（1行に x,y,z,r,g,b,radius,flame。# で始まる行は説明）
	std::ifstream file(kLightsPath);
	if (!file)
	{
		OutputDebugStringA(("【ステージの光源のファイルが読み込めませんでした】" + kLightsPath + "\n").c_str());
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		float v[8] = {};
		std::stringstream ss(line);
		std::string cell;
		int count = 0;
		while (count < 8 && std::getline(ss, cell, ','))
		{
			v[count++] = std::stof(cell);
		}
		if (count < 8) continue;

		Light light;
		light.pos = Math::Vector3(v[0], v[1], v[2]) * kStageScale;
		light.color = { v[3], v[4], v[5] };
		light.radius = v[6] * kStageScale * kLightRadiusRate;
		light.flameSize = v[7] * kStageScale;
		light.phase = static_cast<float>(m_lights.size()) * 1.7f;
		m_lights.push_back(light);
	}
}

float StageLights::CalcFlicker(const Light& light) const
{
	// 速さの違う sin を2つ重ねて、規則正しく見えないようにする
	float t = static_cast<float>(m_frame);
	return 1.0f + kFlickerRange * (0.6f * std::sin(t * kFlickerSpeed1 + light.phase)
		+ 0.4f * std::sin(t * kFlickerSpeed2 + light.phase * 2.3f));
}

void StageLights::Update()
{
	m_frame++;
	m_activeIndices.clear();
	if (m_lights.empty()) return;

	// カメラの近い順に、決まった数だけ点光源として登録する
	// （点光源はフレームの最初に KdAmbientController が空にするので、毎フレーム登録し直す）
	Math::Vector3 camPos = KdShaderManager::Instance().GetCameraCB().CamPos;
	std::vector<std::pair<float, int>> candidates;
	for (int i = 0; i < static_cast<int>(m_lights.size()); ++i)
	{
		float distSq = (m_lights[i].pos - camPos).LengthSquared();
		if (distSq <= kActiveRange * kActiveRange) candidates.push_back({ distSq, i });
	}
	// （距離の 2 乗で比べる。ルートの計算をしなくても、近い・遠いの順番は同じになる）
	std::sort(candidates.begin(), candidates.end());
	if (static_cast<int>(candidates.size()) > kMaxActiveLights) candidates.resize(kMaxActiveLights);

	auto& ambient = KdShaderManager::Instance().WorkAmbientController();
	for (const auto& candidate : candidates)
	{
		const Light& light = m_lights[candidate.second];
		float flicker = CalcFlicker(light);
		ambient.AddPointLight(light.color * (kLightIntensity * flicker), light.radius * flicker, light.pos, true);
		m_activeIndices.push_back(candidate.second);
	}
}

void StageLights::DrawEffect()
{
	if (!m_spFlame || m_activeIndices.empty()) return;

	// カメラの向き（ビュー行列の逆）。炎の板をカメラの方へ向ける（上下はまっすぐ立てたまま）
	Math::Matrix camMat = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	Math::Vector3 camForward = camMat.Backward();
	// （atan2 で、カメラの前の向きを Y 軸の角度にする。+180 度で、板の表をカメラに向ける）
	float yaw = std::atan2(camForward.x, camForward.z);
	Math::Matrix faceCamera = Math::Matrix::CreateRotationY(yaw + DirectX::XM_PI);

	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	for (int index : m_activeIndices)
	{
		const Light& light = m_lights[index];
		float flicker = CalcFlicker(light);
		float width = light.flameSize * (0.9f + 0.2f * flicker);
		float height = width * kFlameAspect * flicker;
		Math::Matrix mWorld = Math::Matrix::CreateScale(width, height, 1.0f) * faceCamera
			* Math::Matrix::CreateTranslation(light.pos - Math::Vector3(0.0f, light.flameSize * 0.5f, 0.0f));

		float bright = kFlameBrightness * flicker;
		Math::Color color = { light.color.x * bright, light.color.y * bright, light.color.z * bright, 1.0f };
		shaderManager.m_StandardShader.DrawPolygon(*m_spFlame, mWorld, color);
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
