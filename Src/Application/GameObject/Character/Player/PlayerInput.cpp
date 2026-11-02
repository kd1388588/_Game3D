// PlayerInput.cpp
// 入力（先行入力・移動の入力）
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

void Player::UpdateInputBuffer()
{
	// 押した瞬間に残りフレームを入れ、押されていない間は1ずつ減らす
	auto update = [](InputHelper::KeyTrigger& key, int& bufferTimer)
	{
		if (key.Update())			bufferTimer = kInputBufferFrame;
		else if (bufferTimer > 0)	bufferTimer--;
	};

	update(m_attackKey, m_attackBuffer);
	update(m_evadeKey, m_evadeBuffer);
	for (int i = 0; i < static_cast<int>(AbilitySlot::Num); ++i)
	{
		update(m_abilityKeys[i], m_abilityBuffer[i]);
	}
}

void Player::ClearInputBuffers()
{
	m_attackBuffer = 0;
	m_evadeBuffer = 0;
	for (int& buffer : m_abilityBuffer) buffer = 0;
}

bool Player::ConsumeInput(int& bufferTimer)
{
	if (bufferTimer <= 0) return false;

	bufferTimer = 0;
	return true;
}

bool Player::IsGuardDashDown() const
{
	return InputHelper::Bind::GuardDash.IsDown();
}

Math::Vector3 Player::GetInputDir() const
{
	Math::Vector3 dir = Math::Vector3::Zero;

	if (IsKeyDown('W')) dir.z += 1.0f;
	if (IsKeyDown('S')) dir.z -= 1.0f;
	if (IsKeyDown('A')) dir.x -= 1.0f;
	if (IsKeyDown('D')) dir.x += 1.0f;

	// パッドの左スティック
	const Math::Vector2& stick = InputHelper::GamePad::GetLeftStick();
	dir.x += stick.x;
	dir.z += stick.y;

	return dir;
}

bool Player::CheckMoveInput() const
{
	return GetInputDir().LengthSquared() > 0.0f;
}

bool Player::GetMoveInputDir(Math::Vector3& outDir) const
{
	// カメラの向きを基準に移動方向を決める
	Math::Matrix cameraRotY = Math::Matrix::Identity;
	if (auto camera = m_camera.lock())
	{
		cameraRotY = camera->GetRotationYMatrix();
	}

	outDir = Math::Vector3::TransformNormal(GetInputDir(), cameraRotY);
	outDir.y = 0.0f;
	if (outDir.LengthSquared() == 0.0f) return false;

	outDir.Normalize();
	return true;
}
