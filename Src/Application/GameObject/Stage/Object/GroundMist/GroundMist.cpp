#include "GroundMist.h"
#include "../../../../Utility/StageSettings.h"
#include "../../../../Utility/GameTime.h"

namespace
{
	const std::string	kTexturePath	= "Asset/Textures/Stage/GroundMist.png";	// Tools/MakeNightSky.py で作成
	constexpr float		kFrameSec		= 1.0f / 60.0f;		// 1フレームの秒数
}

void GroundMist::Init()
{
	m_spPolygon = std::make_shared<KdSquarePolygon>(kTexturePath);
	m_spPolygon->SetPivot(KdSquarePolygon::PivotType::Center_Bottom);	// 下の端を床の高さに合わせる
	m_spPolygon->SetScale(1.0f);
}

void GroundMist::Respawn(Puff& puff, float cx, float cz, bool anywhere) const
{
	const StageSettings::GroundMist& s = StageSettings::Instance().GetGroundMist();

	// anywhere：円の中のどこでもよい（最初に並べる時）
	// それ以外：円のふちに置く（流れて範囲から出たものを、反対側から入ってくるように出し直す）
	float angle = KdRandom::GetFloat(0.0f, DirectX::XM_2PI);
	float dist = anywhere ? s.range * std::sqrt(KdRandom::GetFloat(0.0f, 1.0f)) : s.range * KdRandom::GetFloat(0.85f, 1.0f);
	if (!anywhere)
	{
		// 流れてくる側（流れの向きと反対の半分）に出す
		float drift = DirectX::XMConvertToRadians(s.driftAngle);
		angle = drift + DirectX::XM_PI + KdRandom::GetFloat(-DirectX::XM_PIDIV2, DirectX::XM_PIDIV2);
	}
	puff.pos = { cx + std::sin(angle) * dist, 0.0f, cz + std::cos(angle) * dist };
	puff.size = KdRandom::GetFloat(s.sizeMin, std::max(s.sizeMin, s.sizeMax));
	puff.bright = std::max(0.0f, 1.0f + KdRandom::GetFloat(-1.0f, 1.0f) * (s.brightness > 0.0f ? s.brightnessRandom / s.brightness : 0.0f));
	puff.pos.y = KdRandom::GetFloat(0.0f, s.heightRandom);
	puff.phase = KdRandom::GetFloat(0.0f, DirectX::XM_2PI);
}

void GroundMist::Update()
{
	const StageSettings::GroundMist& s = StageSettings::Instance().GetGroundMist();
	m_time += kFrameSec;

	Math::Vector3 camPos = KdShaderManager::Instance().GetCameraCB().CamPos;

	// 数が変わったら、足りない分を足す・多い分を消す
	int count = std::max(0, s.count);
	while (static_cast<int>(m_puffs.size()) < count)
	{
		Puff puff;
		Respawn(puff, camPos.x, camPos.z, true);
		m_puffs.push_back(puff);
	}
	if (static_cast<int>(m_puffs.size()) > count) m_puffs.resize(count);

	// 流す（スローモーションの時はゆっくり流れる）。範囲の外に出たら、反対側に出し直す
	float drift = DirectX::XMConvertToRadians(s.driftAngle);
	Math::Vector3 move = Math::Vector3(std::sin(drift), 0.0f, std::cos(drift)) * (s.driftSpeed * kFrameSec * GameTime::GetScale());
	for (Puff& puff : m_puffs)
	{
		puff.pos += move;
		float dx = puff.pos.x - camPos.x;
		float dz = puff.pos.z - camPos.z;
		if (dx * dx + dz * dz > s.range * s.range * 1.1f)
		{
			Respawn(puff, camPos.x, camPos.z, false);
		}
	}
}

void GroundMist::DrawEffect()
{
	const StageSettings::GroundMist& s = StageSettings::Instance().GetGroundMist();
	if (!s.enable || !m_spPolygon || m_puffs.empty() || s.brightness <= 0.0f) return;

	// カメラの向き（ビュー行列の逆）。かたまりをカメラの方へ向ける（上下はまっすぐ立てたまま）
	Math::Matrix camMat = KdShaderManager::Instance().GetCameraCB().mView.Invert();
	Math::Vector3 camPos = camMat.Translation();
	Math::Vector3 camForward = camMat.Backward();
	float yaw = std::atan2(camForward.x, camForward.z);
	Math::Matrix faceCamera = Math::Matrix::CreateRotationY(yaw + DirectX::XM_PI);

	// 遠い順に描く（加算なので順番で色は変わらないが、念のため）
	std::vector<std::pair<float, const Puff*>> order;
	order.reserve(m_puffs.size());
	for (const Puff& puff : m_puffs)
	{
		order.push_back({ (puff.pos - camPos).LengthSquared(), &puff });
	}
	std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	for (const auto& entry : order)
	{
		const Puff& puff = *entry.second;

		// カメラに近いものは薄くする
		float dx = puff.pos.x - camPos.x;
		float dz = puff.pos.z - camPos.z;
		float dist = std::sqrt(dx * dx + dz * dz);
		// （nearFadeStart で薄くし始め、そこから nearFadeLength だけ近づくと消える）
		float fadeLength = std::max(s.nearFadeLength, 0.01f);
		float nearRate = std::clamp((dist - (s.nearFadeStart - fadeLength)) / fadeLength, 0.0f, 1.0f);

		// ゆらぎ（ゆっくり濃くなったり薄くなったりする）
		float sway = 1.0f + s.swayRange * std::sin(m_time * s.swaySpeed + puff.phase);

		float bright = s.brightness * puff.bright * sway * nearRate;
		if (bright <= 0.001f) continue;

		Math::Vector3 pos = puff.pos;
		pos.y += s.bottomHeight;
		Math::Matrix mWorld = Math::Matrix::CreateScale(puff.size, puff.size * s.flatten, 1.0f) * faceCamera
			* Math::Matrix::CreateTranslation(pos);

		Math::Color color = { s.color.x * bright, s.color.y * bright, s.color.z * bright, 1.0f };
		shaderManager.m_StandardShader.DrawPolygon(*m_spPolygon, mWorld, color);
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
