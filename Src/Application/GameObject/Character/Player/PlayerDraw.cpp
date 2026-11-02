// PlayerDraw.cpp
// 描画（影・モデル・エフェクト・パニッシャーのオーラ）
// ※Player クラスの関数を、役割ごとにファイルを分けて書いている（クラスの宣言は Player.h）

// Player
#include "Player.h"
#include "PlayerState.h"
#include "PlayerParamManager.h"

// Effect
#include "../../../../Framework/Direct3D/Polygon/KdTrailPolygon.h"

// UI
#include "../UI/Gage/HP/HPGage.h"
#include "../UI/Gage/ATB/ATBGage.h"
#include "../UI/Gage/ATB/ATBCommandMenu.h"

// Enemy
#include "../Enemy/Enemy.h"

// Camera
#include "../../Camera/TPSCamera/TPSCamera.h"

// Effect
#include "../../Effect/EffectPlayer.h"

// Utility
#include "../../../Utility/HitStop.h"
#include "../../../Utility/SoundPlayer.h"
#include "../../../Utility/GameTime.h"

// Scene
#include "../../../Scene/SceneManager.h"
#include "../../../Scene/BaseScene/BaseScene.h"

#include "PlayerConstants.h"

using InputHelper::IsKeyDown;
using namespace PlayerConst;

void Player::GenerateDepthMapFromLight()
{
	// 影：DrawLit と同じように体（スキンメッシュ）と剣を、光から見た深度に描く
	if (!m_model) return;

	auto& shader = KdShaderManager::Instance().m_StandardShader;
	shader.DrawModel(*m_model, m_mWorld);
	if (m_swordModel)
	{
		shader.DrawModel(*m_swordModel, m_swordWorldR);
		shader.DrawModel(*m_swordModel, m_swordWorldL);
	}
}

void Player::DrawEffect()
{
	auto& shaderManager = KdShaderManager::Instance();

	// 軌跡は他のオブジェクトを描いた後に、加算合成・深度書き込みなしで描く
	// （帯の暗い部分が後ろの物を隠さないように）
	// 裏からも見えるようにカリングも切る
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	for (const SwordTrail* pTrail : { &m_swordTrailR, &m_swordTrailL })
	{
		if (pTrail->polygon)
		{
			shaderManager.m_StandardShader.DrawPolygon(*pTrail->polygon);
		}
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}

void Player::DrawLit()
{
	if (!m_model) return;

	auto& shaderManager = KdShaderManager::Instance();
	auto& shader = shaderManager.m_StandardShader;

	shaderManager.ChangeRasterizerState(KdRasterizerState::CullNone);
	shaderManager.ChangeBlendState(KdBlendState::Alpha);

	// リムライト：オペレーターは青、パニッシャーはオレンジ
	// いつもは体全体のふちを薄く光らせておき、モードを切り替えた時だけ強く光らせる
	const Math::Vector3& modeLimColor = IsPunisherMode() ? kPunisherLimColor : kOperatorLimColor;
	shader.SetLimLightEnable(true);
	shader.SetLimLight(modeLimColor * kBaseLimBright, kBaseLimPow);

	if (m_isLimLight)
	{
		// 後半は、いつもの薄いリムライトへなめらかに戻す
		// （進み具合 0～1 を、始めと終わりがゆっくりになる曲線で 1 → 0 に変え、色と鋭さを混ぜる）
		float rate = std::clamp((m_limPow / kLimPowMax - kLimFadeStartRate) / (1.0f - kLimFadeStartRate), 0.0f, 1.0f);
		float fade = 1.0f - rate * rate * (3.0f - 2.0f * rate);

		float bright = kBaseLimBright + (1.0f - kBaseLimBright) * fade;
		float pow = kBaseLimPow + (m_limPow - kBaseLimPow) * fade;
		shader.SetLimLight(modeLimColor * bright, pow);
	}

	shader.DrawModel(*m_model, m_mWorld);

	if (m_swordModel)
	{
		shader.DrawModel(*m_swordModel, m_swordWorldR);
		shader.DrawModel(*m_swordModel, m_swordWorldL);
	}

	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();

	// パニッシャーのオーラ（体を描いた後に重ねる）
	DrawPunisherAura();
}

Math::Matrix Player::CreateGuardEffectMatrix() const
{
	// 体の前（胸の高さ）に、プレイヤーの向きに合わせて盾を立てる
	Math::Matrix rotY = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), rotY);
	Math::Vector3 pos = m_pos + Math::Vector3(0.0f, kGuardEffectHeight, 0.0f) + forward * kGuardEffectForward;
	return rotY * Math::Matrix::CreateTranslation(pos);
}

void Player::DrawPunisherAura()
{
	if (!m_model || m_auraRate <= 0.0f) return;

	// 腰を中心に大きくする
	Math::Vector3 center = m_pos;
	if (m_pelvisNode >= 0 && m_pelvisNode < static_cast<int>(m_model->GetNodes().size()))
	{
		center = Math::Vector3::Transform(m_model->GetNodes()[m_pelvisNode].m_worldTransform.Translation(), m_mWorld);
	}

	// 裏側だけ・加算合成・深度書き込みなしで描くと、体のふちのまわりだけが光って見える
	auto& shaderManager = KdShaderManager::Instance();
	shaderManager.ChangeRasterizerState(KdRasterizerState::CullFront);
	shaderManager.ChangeBlendState(KdBlendState::Add);
	shaderManager.ChangeDepthStencilState(KdDepthStencilState::ZWriteDisable);

	float frame = static_cast<float>(m_auraFrame);
	for (int i = 0; i < kAuraLayerNum; ++i)
	{
		float layer = static_cast<float>(i);

		// 1枚ごとにずらしたゆらぎで、大きさと明るさを細かく変える（炎のようにちらつかせる）
		float wave = std::sin(frame * kAuraWaveSpeed + layer * 2.1f);
		float flicker = 1.0f - kAuraFlickerRange * (0.5f + 0.5f * std::sin(frame * kAuraFlickerSpeed + layer * 1.7f));

		float scaleXZ = kAuraBaseScale + kAuraScaleStep * layer + kAuraWaveScale * wave;
		float scaleY = scaleXZ + kAuraStretchY * layer;
		Math::Vector3 rise = { 0.0f, kAuraRiseY * layer * (1.0f + 0.5f * wave), 0.0f };

		Math::Matrix auraMat = m_mWorld
			* Math::Matrix::CreateTranslation(-center)
			* Math::Matrix::CreateScale(scaleXZ, scaleY, scaleXZ)
			* Math::Matrix::CreateTranslation(center + rise);

		// 外側ほど薄く（体の色は使わず、光の色だけで描く）
		Math::Vector3 color = kAuraColor * (m_auraRate * flicker / (layer + 1.0f));
		shaderManager.m_StandardShader.DrawModel(*m_model, auraMat, Math::Color(0.0f, 0.0f, 0.0f, 1.0f), color);
	}

	shaderManager.UndoDepthStencilState();
	shaderManager.UndoBlendState();
	shaderManager.UndoRasterizerState();
}
