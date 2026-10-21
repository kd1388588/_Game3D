#pragma once
#include "../CameraBase.h"

class TPSCamera : public CameraBase
{
public:
	TPSCamera()							{}
	~TPSCamera()			override	{}

	void Init()				override;
	void PostUpdate()		override;
	
private:

	// プレイヤーが左右に動いた時に、カメラもその方向へ付いていく
	// （少しだけ横に回り込み、進む方向が見えるように注視点も少し先へずらす）
	void UpdateSideFollow(const Math::Vector3& moveDelta);

	// ロックオン中は、プレイヤーの後ろから敵の方を見るようにカメラを回す
	void UpdateLockOnFollow(const Math::Vector3& playerPos, const Math::Vector3& targetPos);

	// ヒットストップ中にカメラを揺らす
	void ApplyHitShake();

	Math::Vector3	m_lookAhead = Math::Vector3::Zero;	// 注視点を進む方向へずらす量（なめらかに変える）
};