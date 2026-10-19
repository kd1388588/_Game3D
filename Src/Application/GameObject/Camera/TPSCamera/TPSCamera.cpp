#include "TPSCamera.h"
#include "../../Character/Player/Player.h"
#include "../../Character/Enemy/Enemy.h"
#include "../../../Utility/HitStop.h"
#include "../../../Scene/SceneManager.h"

namespace
{
	// 左右に動いた時にカメラが付いていく強さ
	constexpr float kSideTurnRate		= 8.0f;		// 横に動いた量 × これ（度）だけ、カメラを横に回す
	constexpr float kMaxSideTurn		= 1.5f;		// 1フレームに回す角度の上限（度）
	constexpr float kLookAheadRate		= 3.0f;		// 横に動いた量 × これだけ、注視点を進む方向へずらす
	constexpr float kMaxLookAhead		= 0.6f;		// 注視点をずらす量の上限
	constexpr float kLookAheadFollow	= 0.08f;	// 注視点をずらす速さ（0～1。小さいほどゆっくり）

	// ロックオン中
	constexpr float kLockOnFollowRate	= 0.12f;	// 敵の方へカメラを回す速さ（0～1）
	constexpr float kLockOnPitch		= 12.0f;	// 少し上から見下ろす角度（度）
	constexpr float kLockOnPitchRate	= 0.05f;

	// めり込み防止
	constexpr float kCameraWallMargin	= 0.3f;		// 壁からこれだけ手前にカメラを置く
	constexpr float kCameraMinDistance	= 0.5f;		// キャラにこれ以上は近づけない
}

void TPSCamera::Init()
{
	// 親クラスの初期化呼び出し
	CameraBase::Init();

	// 注視点からのカメラの位置（少し引いて、周りの敵が見やすいようにする）
	m_mLocalPos = Math::Matrix::CreateTranslation(0, 1.3f, -5.5f);

	//SetCursorPos(m_FixMousePos.x, m_FixMousePos.y);
}

void TPSCamera::PostUpdate()
{
	Math::Matrix								_targetMat = Math::Matrix::Identity;
	const std::shared_ptr<const KdGameObject>	_spTarget = m_wpTarget.lock();
	if (_spTarget)
	{
		Math::Vector3 targetPos = _spTarget->GetPos();

		std::shared_ptr<const Player> player = std::dynamic_pointer_cast<const Player>(_spTarget);
		if (player)
		{
			targetPos = player->GetCameraTargetPos();

			if (auto lockTarget = player->GetLockTarget())
			{
				// ロックオン中：プレイヤーの後ろから、敵の方を見る
				UpdateLockOnFollow(player->GetPos(), lockTarget->GetPos());
				m_lookAhead *= 0.9f;
			}
			else
			{
				// 右に動いたらカメラも右へ（左も同じ）
				UpdateSideFollow(player->GetLastMoveDelta());
			}
			targetPos += m_lookAhead;
		}

		_targetMat = Math::Matrix::CreateTranslation(targetPos);
	}
	// カメラの回転
	if (!(GetAsyncKeyState('Q') & 0x8000))
	{
		UpdateRotateByMouse();
	}

	m_mRotation = GetRotationMatrix();
	m_mWorld = m_mLocalPos * m_mRotation * _targetMat;



	// めり込み防止：注視点（キャラ）からカメラへレイを飛ばし、間に建物や地面があれば、その手前にカメラを寄せる
	// （キャラから飛ばして一番手前の壁を使うので、壁が何枚あってもキャラが隠れない）
	if (_spTarget)
	{
		Math::Vector3 start = _targetMat.Translation();
		Math::Vector3 toCamera = GetPos() - start;
		float distance = toCamera.Length();
		if (distance > 0.001f)
		{
			KdCollider::RayInfo rayInfo;
			rayInfo.m_pos = start;
			rayInfo.m_dir = toCamera / distance;
			rayInfo.m_range = distance;
			rayInfo.m_type = KdCollider::TypeGround;

			float nearest = distance;
			for (auto& obj : SceneManager::Instance().GetObjList())
			{
				if (obj.get() == this || obj == _spTarget) continue;

				std::list<KdCollider::CollisionResult> results;
				if (!obj->Intersects(rayInfo, &results)) continue;

				for (auto& ret : results)
				{
					nearest = std::min(nearest, (ret.m_hitPos - start).Length());
				}
			}

			if (nearest < distance)
			{
				// 壁から少し手前（キャラ側）に置く
				SetPos(start + rayInfo.m_dir * std::max(nearest - kCameraWallMargin, kCameraMinDistance));
			}
		}
	}

	// ヒットストップ中は、カメラを少し揺らす（めり込み防止の補正の後に行う）
	ApplyHitShake();
}

void TPSCamera::UpdateSideFollow(const Math::Vector3& moveDelta)
{
	// カメラから見た「右」の向き
	Math::Vector3 right = Math::Vector3::TransformNormal(Math::Vector3(1.0f, 0.0f, 0.0f), GetRotationYMatrix());

	// 横にどれだけ動いたか（右が＋、左が－）
	Math::Vector3 move = moveDelta;
	move.y = 0.0f;
	float side = move.Dot(right);

	// カメラを動いた方向へ少しだけ回す（右へ走り続けると、カメラがゆっくり右へ回り込む）
	m_DegAng.y += std::clamp(side * kSideTurnRate, -kMaxSideTurn, kMaxSideTurn);

	// 注視点を動いた方向へ少しずらす（進む先が見えやすくなる）
	float offset = std::clamp(side * kLookAheadRate, -kMaxLookAhead, kMaxLookAhead);
	Math::Vector3 target = right * offset;
	m_lookAhead += (target - m_lookAhead) * kLookAheadFollow;
}

void TPSCamera::UpdateLockOnFollow(const Math::Vector3& playerPos, const Math::Vector3& targetPos)
{
	Math::Vector3 dir = targetPos - playerPos;
	dir.y = 0.0f;
	if (dir.LengthSquared() <= 0.0001f) return;

	// 敵の方向の角度（度）へ、近い回り方でなめらかに回す
	float targetYaw = DirectX::XMConvertToDegrees(std::atan2(dir.x, dir.z));
	float diff = targetYaw - m_DegAng.y;
	while (diff > 180.0f)	diff -= 360.0f;
	while (diff < -180.0f)	diff += 360.0f;
	m_DegAng.y += diff * kLockOnFollowRate;

	// 上下は少し見下ろす角度へ
	m_DegAng.x += (kLockOnPitch - m_DegAng.x) * kLockOnPitchRate;
}

void TPSCamera::ApplyHitShake()
{
	float power = HitStop::GetShakePower();
	if (power <= 0.0f) return;

	// カメラの右・上の向きに、ランダムに少しずらす
	Math::Vector3 shake =
		m_mWorld.Right() * KdRandom::GetFloat(-power, power) +
		m_mWorld.Up() * KdRandom::GetFloat(-power, power);
	m_mWorld.Translation(m_mWorld.Translation() + shake);
}
