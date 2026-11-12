#pragma once
#include "../CameraBase.h"

// ============================================================
// 【役割】ゲーム中の三人称カメラ（プレイヤーの後ろ・少し上から映す）
// 【構成】PostUpdate（全員が動き終わった後）で、次の順に決める
//   ① 注視点：プレイヤーの足元 ＋ 進む先へのずらし（m_lookAhead）
//   ② 回転：マウス・右スティック（ロックオン中は敵の方へ、横に動いた時はその方向へ少し回す）
//   ③ 位置：注視点から、回転した向きに、決まった距離だけ下がった所
//   ④ めり込み防止：キャラからカメラへレイを飛ばし、壁があればその手前に寄せる
//   ⑤ ヒットストップ中の揺れ
// 【なぜ PostUpdate か】プレイヤーが動き終わった後の位置を映すため（Update で決めると 1 フレーム遅れる）
// 【参考】三人称カメラの考え方：Mark Haigh-Hutchinson『Real-Time Cameras』（ゲームのカメラの本）
// ============================================================
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