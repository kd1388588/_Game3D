#include "TitleCamera.h"
#include "../TitleChara/TitleChara.h"
#include "../../../Utility/SpriteUtil.h"

#include <cmath>	// std::lerp

namespace
{
	// 登場演出（遠く・高い位置から寄ってくる）
	constexpr int	kIntroFrame			= 180;
	constexpr float	kIntroDistance		= 7.5f;		// 登場演出の開始時の距離
	constexpr float	kIntroHeight		= 2.5f;		// 登場演出の開始時の高さ（注目位置から）
	constexpr float	kIntroYawDeg		= -50.0f;	// 登場演出の開始時の回り込み角度（度）

	// 登場後の揺れ
	constexpr float	kDistance			= 3.4f;		// キャラクターからの距離
	constexpr float	kDistanceSwing		= 0.35f;	// 距離の揺れ幅
	constexpr float	kHeight				= 0.25f;	// 注目位置からの高さ
	constexpr float	kHeightSwing		= 0.2f;		// 高さの揺れ幅
	constexpr float	kYawSwingDeg		= 28.0f;	// 左右に回り込む角度の揺れ幅（度）
	constexpr float	kYawSpeed			= 0.0045f;	// 回り込みの速さ（ラジアン/フレーム）
	constexpr float	kDistanceSpeed		= 0.0031f;	// 距離の揺れの速さ
	constexpr float	kHeightSpeed		= 0.0057f;	// 高さの揺れの速さ

	// キャラクターを画面の右寄りに映すため、注視点を左にずらす量
	// （左のメニューとキャラクターのエフェクトが重ならないよう、大きめにしている）
	constexpr float	kFrameOffset		= 1.35f;
}

void TitleCamera::Init()
{
	CameraBase::Init();
	m_frame = 0;
}

void TitleCamera::PostUpdate()
{
	m_frame++;

	Math::Vector3 lookAt = Math::Vector3(0.0f, 1.1f, 0.0f);
	if (auto chara = m_wpChara.lock())
	{
		lookAt = chara->GetLookAtPos();
	}

	// 登場後の揺れ（周期の違う sin を組み合わせて、同じ動きの繰り返しに見えにくくする）
	float t = static_cast<float>(m_frame);
	float yaw = DirectX::XMConvertToRadians(kYawSwingDeg) * std::sin(t * kYawSpeed);
	float distance = kDistance + kDistanceSwing * std::sin(t * kDistanceSpeed);
	float height = kHeight + kHeightSwing * std::sin(t * kHeightSpeed);

	// 登場演出：遠く・高い位置から、ゆっくり寄っていく
	float intro = SpriteUtil::EaseOutCubic(static_cast<float>(m_frame) / kIntroFrame);
	yaw = std::lerp(DirectX::XMConvertToRadians(kIntroYawDeg), yaw, intro);
	distance = std::lerp(kIntroDistance, distance, intro);
	height = std::lerp(kIntroHeight, height, intro);

	// キャラクターの正面（-Z側）から映す
	Math::Vector3 dir = { std::sin(yaw), 0.0f, -std::cos(yaw) };
	Math::Vector3 eye = lookAt + dir * distance + Math::Vector3(0.0f, height, 0.0f);

	// 注視点をカメラから見て左にずらし、キャラクターを右寄りに映す
	Math::Vector3 forward = lookAt - eye;
	forward.Normalize();
	Math::Vector3 right = Math::Vector3::Up.Cross(forward);
	right.Normalize();
	Math::Vector3 target = lookAt - right * kFrameOffset;

	// ビュー行列（左手座標系）の逆行列がカメラのワールド行列
	Math::Matrix view = DirectX::XMMatrixLookAtLH(eye, target, Math::Vector3::Up);
	m_mWorld = view.Invert();
}
