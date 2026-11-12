#include "HealFountain.h"
#include "../../../Character/Player/Player.h"

namespace
{
	// 画像（Tools/MakeKeyTexture.py で作成）
	const std::string	kWaterTexPath	= "Asset/Textures/Stage/FountainWater.png";
	const std::string	kShineTexPath	= "Asset/Textures/Stage/FountainShine.png";
	const std::string	kStreamTexPath	= "Asset/Textures/Stage/FountainStream.png";
	const std::string	kRingTexPath	= "Asset/Textures/Stage/FountainRing.png";
	const std::string	kGlowTexPath	= "Asset/Textures/Stage/FountainGlow.png";

	// 形（ステージのモデルの器に合わせる。モデルの座標 × 1.25）
	constexpr float		kWaterHeight	= 0.75f;	// 水面の高さ（ふちの高さ 1.06m より少し低い）
	constexpr float		kWaterSize		= 4.2f;		// 水面の直径（ふちの内側。丸く切り抜く）
	constexpr float		kWaterTurnSpeed	= 0.0015f;	// 水面の模様を回す速さ（ラジアン/フレーム）
	constexpr float		kShineTurnSpeed	= -0.0025f;	// きらめきは反対へ回す（2 つの模様が重なって、揺らいで見える）

	// 流れ落ちる水（上の皿のふちから、水面まで）
	constexpr int		kStreamCount	= 6;		// 皿のまわりに何本流すか
	constexpr float		kStreamRadius	= 1.15f;	// 皿のふちの半径
	constexpr float		kStreamTop		= 2.45f;	// 皿のふちの高さ
	constexpr float		kStreamWidth	= 0.5f;
	constexpr float		kStreamSpeed	= 0.025f;	// 模様を下へ流す速さ（UV/フレーム）

	// 波紋
	constexpr int		kRippleInterval	= 14;		// 何フレームごとに出すか
	constexpr int		kRippleLife		= 50;		// 広がりきって消えるまでのフレーム数
	constexpr float		kRippleMaxSize	= 1.3f;		// 一番大きくなった時の直径（m）

	// 光の柱（光っている時だけ）
	const Math::Vector2	kBeamSize		= { 2.6f, 4.5f };

	// 使う判定（器のまわりは見えない壁で近づけないので、それより少し広く）
	constexpr float		kUseRange		= 4.6f;		// 泉の真ん中から、横の距離がこれより近いと使う
	constexpr float		kUseHeight		= 3.0f;		// 高さの差がこれより小さい時だけ

	// 光の強さ
	constexpr float		kSleepGlow		= 0.0f;		// 眠っている時・使った後
	constexpr float		kGlowFollow		= 0.04f;	// 光の強さが変わる速さ（1 フレームに近づく割合）
	const Math::Vector3	kColor			= { 0.35f, 0.85f, 1.0f };	// 水色
	constexpr float		kLightIntensity	= 2.5f;
	constexpr float		kLightRadius	= 9.0f;
}

void HealFountain::Init()
{
	auto create = [](const std::string& path, KdSquarePolygon::PivotType pivot)
	{
		auto poly = std::make_shared<KdSquarePolygon>(path);
		poly->SetPivot(pivot);
		poly->SetScale(1.0f);
		return poly;
	};
	m_spWater	= create(kWaterTexPath, KdSquarePolygon::PivotType::Center_Middle);
	m_spShine	= create(kShineTexPath, KdSquarePolygon::PivotType::Center_Middle);
	m_spStream	= create(kStreamTexPath, KdSquarePolygon::PivotType::Center_Bottom);
	m_spRing	= create(kRingTexPath, KdSquarePolygon::PivotType::Center_Middle);
	m_spBeam	= create(kGlowTexPath, KdSquarePolygon::PivotType::Center_Bottom);

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

	// 流れ落ちる水の模様を下へずらす（1 を超えたら戻しても、模様は上下でつながっているので見た目は同じ）
	m_streamScroll += kStreamSpeed;
	if (m_streamScroll >= 1.0f) m_streamScroll -= 1.0f;
	if (m_spStream) m_spStream->SetUVRect({ 0.0f, -m_streamScroll }, { 1.0f, 1.0f - m_streamScroll });

	// 波紋：水が落ちる所（皿のふちの真下）に、決まった間隔で出し、古いものから消す
	if (m_frame % kRippleInterval == 0)
	{
		int index = KdRandom::GetInt(0, kStreamCount - 1);
		float angle = DirectX::XM_2PI * index / kStreamCount + KdRandom::GetFloat(-0.2f, 0.2f);
		float radius = kStreamRadius + KdRandom::GetFloat(-0.1f, 0.15f);
		Ripple ripple;
		ripple.pos = m_basePos + Math::Vector3(std::cos(angle) * radius, kWaterHeight + 0.01f, std::sin(angle) * radius);
		m_ripples.push_back(ripple);
	}
	for (Ripple& ripple : m_ripples) ripple.age++;
	m_ripples.erase(std::remove_if(m_ripples.begin(), m_ripples.end(),
		[](const Ripple& r) { return r.age >= kRippleLife; }), m_ripples.end());

	// 水色の光（光っている時だけ、まわりを照らす。点光源は毎フレーム登録し直す）
	if (m_glowRate > 0.05f)
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

Math::Matrix HealFountain::CreateFlatMatrix(float size, float rotY, const Math::Vector3& pos) const
{
	// 板（XY の面）を X 軸で 90 度回して、床と同じ向きに寝かせる（表は上を向く）
	return Math::Matrix::CreateScale(size, size, 1.0f)
		* Math::Matrix::CreateRotationX(DirectX::XM_PIDIV2)
		* Math::Matrix::CreateRotationY(rotY)
		* Math::Matrix::CreateTranslation(pos);
}

void HealFountain::BeginAdditive() const
{
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);
}

void HealFountain::EndAdditive() const
{
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

void HealFountain::DrawLit()
{
	if (!m_spWater) return;

	// 水面：ふつうの物と同じように光で照らす（松明の光で水が見える）。光っている時は少し水色を強くする
	Math::Matrix water = CreateFlatMatrix(kWaterSize, m_frame * kWaterTurnSpeed, m_basePos + Math::Vector3(0.0f, kWaterHeight, 0.0f));
	float tint = 1.0f + 0.4f * m_glowRate;
	Math::Color color = { 1.0f, tint, tint, 1.0f };

	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.m_StandardShader.DrawPolygon(*m_spWater, water, color);
	shaderManager.UndoRasterizerState();
}

void HealFountain::DrawEffect()
{
	if (!m_spShine || !m_spStream || !m_spRing || !m_spBeam) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	BeginAdditive();

	float pulse = 0.85f + 0.15f * std::sin(m_frame * 0.06f);
	Math::Vector3 waterCenter = m_basePos + Math::Vector3(0.0f, kWaterHeight + 0.02f, 0.0f);

	// ① 水面のきらめき（ふだんは控えめ、光っている時は強く）
	float shine = (0.35f + 0.9f * m_glowRate) * pulse;
	shader.DrawPolygon(*m_spShine, CreateFlatMatrix(kWaterSize, m_frame * kShineTurnSpeed, waterCenter),
		Math::Color(shine, shine, shine, 1.0f));

	// ② 流れ落ちる水：皿のまわりに、カメラの方を向けた縦の板を並べる（Y 軸だけ回して、まっすぐ立てる）
	Math::Matrix camMat = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	Math::Vector3 camForward = camMat.Backward();
	Math::Matrix faceCamera = Math::Matrix::CreateRotationY(std::atan2(camForward.x, camForward.z) + DirectX::XM_PI);
	float streamHeight = kStreamTop - kWaterHeight;
	float streamBright = 0.55f + 0.5f * m_glowRate;
	for (int i = 0; i < kStreamCount; ++i)
	{
		float angle = DirectX::XM_2PI * i / kStreamCount;
		Math::Vector3 pos = m_basePos + Math::Vector3(std::cos(angle) * kStreamRadius, kWaterHeight, std::sin(angle) * kStreamRadius);
		Math::Matrix mWorld = Math::Matrix::CreateScale(kStreamWidth, streamHeight, 1.0f) * faceCamera * Math::Matrix::CreateTranslation(pos);
		shader.DrawPolygon(*m_spStream, mWorld, Math::Color(streamBright, streamBright, streamBright, 1.0f));
	}

	// ③ 波紋：広がりながら薄くなる
	for (const Ripple& ripple : m_ripples)
	{
		float t = static_cast<float>(ripple.age) / kRippleLife;
		float size = kRippleMaxSize * (0.2f + 0.8f * t);
		float bright = (1.0f - t) * (0.6f + 0.6f * m_glowRate);
		shader.DrawPolygon(*m_spRing, CreateFlatMatrix(size, 0.0f, ripple.pos), Math::Color(bright, bright, bright, 1.0f));
	}

	// ④ 光の柱（光っている時だけ）
	if (m_glowRate > 0.05f)
	{
		Math::Matrix beam = Math::Matrix::CreateScale(kBeamSize.x, kBeamSize.y, 1.0f) * faceCamera * Math::Matrix::CreateTranslation(waterCenter);
		float b = 0.6f * m_glowRate * pulse;
		shader.DrawPolygon(*m_spBeam, beam, Math::Color(kColor.x * b, kColor.y * b, kColor.z * b, 1.0f));
	}

	EndAdditive();
}
