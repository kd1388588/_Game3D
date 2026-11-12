#include "HealFountain.h"
#include "../../../Character/Player/Player.h"

namespace
{
	const std::string	kGlowTexPath	= "Asset/Textures/Stage/FountainGlow.png";	// Tools/MakeKeyTexture.py で作成

	// 形（ステージのモデルの器に合わせる。モデルの座標 × 1.25）
	constexpr float		kWaterHeight	= 0.86f;	// 水面の高さ（器の上の面のすぐ上）
	constexpr float		kWaterSize		= 4.2f;		// 水面の大きさ（器の内側。直径 m）
	const Math::Vector2	kBeamSize		= { 2.6f, 4.5f };	// 立ちのぼる光の大きさ（幅・高さ）

	// 使う判定（器のまわりは見えない壁で近づけないので、それより少し広く）
	constexpr float		kUseRange		= 4.5f;		// 泉の真ん中から、横の距離がこれより近いと使う
	constexpr float		kUseHeight		= 3.0f;		// 高さの差がこれより小さい時だけ

	// 光の強さ
	constexpr float		kSleepGlow		= 0.15f;	// 眠っている時・使った後（水がうっすら見える）
	constexpr float		kGlowFollow		= 0.04f;	// 光の強さが変わる速さ（1 フレームに近づく割合）
	const Math::Vector3	kColor			= { 0.35f, 0.85f, 1.0f };	// 水色
	constexpr float		kLightIntensity	= 2.5f;
	constexpr float		kLightRadius	= 9.0f;
}

void HealFountain::Init()
{
	m_spWater = std::make_shared<KdSquarePolygon>(kGlowTexPath);
	m_spWater->SetPivot(KdSquarePolygon::PivotType::Center_Middle);
	m_spWater->SetScale(1.0f);

	m_spBeam = std::make_shared<KdSquarePolygon>(kGlowTexPath);
	m_spBeam->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);
	m_spBeam->SetScale(1.0f);

	m_glowRate = kSleepGlow;
}

void HealFountain::Activate()
{
	if (m_state != State::Sleep) return;
	m_state = State::Active;
}

void HealFountain::Update()
{
	m_frame++;

	// 光の強さを、今の状態の強さへなめらかに近づける
	float target = (m_state == State::Active) ? 1.0f : kSleepGlow;
	m_glowRate += (target - m_glowRate) * kGlowFollow;

	// 水色の光（光っている時だけ、まわりを照らす。点光源は毎フレーム登録し直す）
	if (m_glowRate > 0.2f)
	{
		float pulse = 0.85f + 0.15f * std::sin(m_frame * 0.06f);
		KdShaderManager::Instance().WorkAmbientController().AddPointLight(
			kColor * (kLightIntensity * m_glowRate * pulse), kLightRadius,
			m_basePos + Math::Vector3(0.0f, kWaterHeight + 1.0f, 0.0f), true);
	}

	if (m_state != State::Active) return;

	// 光っている時に、プレイヤーが近づいたら回復する（1 度だけ）
	auto player = m_wpPlayer.lock();
	if (!player || !player->IsAlive()) return;

	Math::Vector3 diff = player->GetPos() - m_basePos;
	float height = std::abs(diff.y);
	diff.y = 0.0f;
	if (diff.LengthSquared() <= kUseRange * kUseRange && height <= kUseHeight)
	{
		player->RecoverAll();
		m_state = State::Used;
		m_isJustUsed = true;
	}
}

void HealFountain::DrawEffect()
{
	if (!m_spWater || !m_spBeam) return;

	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	float pulse = 0.85f + 0.15f * std::sin(m_frame * 0.06f);

	// 水面：板（XY の面）を X 軸で 90 度回して、床と同じ向きに寝かせる。ゆっくり回して、水の輪がゆらめくようにする
	Math::Matrix water = Math::Matrix::CreateScale(kWaterSize, kWaterSize, 1.0f)
		* Math::Matrix::CreateRotationX(DirectX::XM_PIDIV2)
		* Math::Matrix::CreateRotationY(m_frame * 0.004f)
		* Math::Matrix::CreateTranslation(m_basePos + Math::Vector3(0.0f, kWaterHeight, 0.0f));
	float waterBright = (0.35f + 0.65f * m_glowRate) * pulse;
	Math::Color waterColor = { kColor.x * waterBright, kColor.y * waterBright, kColor.z * waterBright, 1.0f };
	shaderManager.m_StandardShader.DrawPolygon(*m_spWater, water, waterColor);

	// 立ちのぼる光：光っている時だけ。カメラの方へ向ける（Y 軸だけ回して、上下はまっすぐ立てる）
	if (m_glowRate > 0.2f)
	{
		Math::Matrix camMat = shaderManager.GetCameraCB().mView.Invert();
		Math::Vector3 camForward = camMat.Backward();
		float yaw = std::atan2(camForward.x, camForward.z);
		Math::Matrix beam = Math::Matrix::CreateScale(kBeamSize.x, kBeamSize.y, 1.0f)
			* Math::Matrix::CreateRotationY(yaw + DirectX::XM_PI)
			* Math::Matrix::CreateTranslation(m_basePos + Math::Vector3(0.0f, kWaterHeight, 0.0f));
		float beamBright = 0.6f * m_glowRate * pulse;
		Math::Color beamColor = { kColor.x * beamBright, kColor.y * beamBright, kColor.z * beamBright, 1.0f };
		shaderManager.m_StandardShader.DrawPolygon(*m_spBeam, beam, beamColor);
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
