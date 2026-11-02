// PlayerMove.cpp
// 移動（向き・ロックオン・敵との押し出し・足音・ガード歩き・ジャンプの解析）
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

std::shared_ptr<Enemy> Player::FindLockTarget(float range) const
{
	std::shared_ptr<Enemy> nearest = nullptr;
	float nearestDist = range;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || !enemy->IsAlive() || enemy->IsExpired()) continue;

		Math::Vector3 diff = enemy->GetPos() - m_pos;
		diff.y = 0.0f;
		float dist = diff.Length();
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearest = enemy;
		}
	}
	return nearest;
}

void Player::UpdateLockOn()
{
	// ロックオンは自動：10m 以内の一番近い敵にロックオンし、倒した・離れた時は外して探し直す
	auto target = m_wpLockTarget.lock();
	if (target)
	{
		Math::Vector3 diff = target->GetPos() - m_pos;
		diff.y = 0.0f;
		if (!target->IsAlive() || target->IsExpired() || diff.Length() > kLockOnRange + kLockOnLoseMargin)
		{
			m_wpLockTarget.reset();
			target = nullptr;
		}
	}

	if (!target)
	{
		m_wpLockTarget = FindLockTarget(kLockOnRange);
	}
}

void Player::FaceLockTarget()
{
	auto target = m_wpLockTarget.lock();
	if (!target) return;

	Math::Vector3 dir = target->GetPos() - m_pos;
	dir.y = 0.0f;
	if (dir.LengthSquared() <= 0.0001f) return;
	dir.Normalize();
	TurnToward(dir, 180.0f);
}

void Player::ApplyAnimationMotion()
{
	if (!m_useRootMotion) return;

	// 腰の移動量はモデルから見た向きなので、モデルの向き（m_angle + 180度）で回してワールドの向きにする
	Math::Matrix modelRot = Math::Matrix::CreateRotationY(DirectX::XMConvertToRadians(m_angle + 180.0f));
	Math::Vector3 move = Math::Vector3::TransformNormal(m_rootMoveDelta, modelRot) * m_rootScale;

	// 上下の移動は重力と地面判定に任せる
	move.y = 0.0f;
	m_pos += move;
}

void Player::PushOutFromEnemies()
{
	// 前のフレームの位置（Update の最初で記録したもの）から今の位置までの動きで調べる
	// （攻撃の踏み込みで1フレームに大きく動いても、敵の体を通り抜けないようにする）
	Math::Vector3 prev = m_prevFramePos;
	prev.y = 0.0f;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		auto enemy = std::dynamic_pointer_cast<Enemy>(obj);
		if (!enemy || !enemy->IsAlive() || enemy->IsExpired()) continue;

		Math::Vector3 center = enemy->GetPos();
		center.y = 0.0f;
		float radius = enemy->GetBodyScale() * kEnemyBodyRadiusRate + kPlayerBodyRadius;

		Math::Vector3 cur = m_pos;
		cur.y = 0.0f;
		Math::Vector3 toCur = cur - center;
		Math::Vector3 toPrev = prev - center;

		Math::Vector3 dir;
		if (toCur.LengthSquared() < radius * radius)
		{
			// ① 今、敵の体の中にいる：今の位置から一番近い体の外へ押し出す
			//    （横や後ろへ離れる動きはそのまま残るので、離れようとすれば離れられる）
			//    ただし、敵の中心を越えて反対側へ行った時は、前にいた側へ戻す（回り込み防止）
			dir = toCur;
			if (toPrev.LengthSquared() > 0.0001f && toPrev.Dot(toCur) < 0.0f) dir = toPrev;
		}
		else
		{
			// ② 今は体の外：1フレームで体を飛び越えて、反対側に出ていないかだけ調べる
			//    （前の位置が体の外で、今の位置との間の線が体の中を通り、前と今が敵の反対側にある時）
			if (toPrev.LengthSquared() < radius * radius) continue;
			if (toPrev.Dot(toCur) >= 0.0f) continue;

			Math::Vector3 move = cur - prev;
			float moveLenSq = move.LengthSquared();
			if (moveLenSq < 0.0001f) continue;
			float t = std::clamp((center - prev).Dot(move) / moveLenSq, 0.0f, 1.0f);
			Math::Vector3 closest = prev + move * t;
			if ((closest - center).LengthSquared() >= radius * radius) continue;

			dir = toPrev;
		}

		if (dir.LengthSquared() < 0.0001f) dir = Math::Vector3(0.0f, 0.0f, -1.0f);
		dir.Normalize();

		m_pos.x = center.x + dir.x * radius;
		m_pos.z = center.z + dir.z * radius;
	}
}

void Player::UpdateFootsteps()
{
	if (m_footstepCoolTimer > 0) m_footstepCoolTimer--;
	if (!m_model) return;

	// 走る・歩く状態の時だけ鳴らす（攻撃や回避の足の動きでは鳴らさない）
	float volume = m_state ? m_state->GetFootstepVolume() : 0.0f;

	const auto& nodes = m_model->GetNodes();
	for (int f = 0; f < 2; ++f)
	{
		int node = m_footstepNodes[f];
		if (node < 0 || node >= static_cast<int>(nodes.size())) continue;

		float footY = nodes[node].m_worldTransform.Translation().y;

		// 地面に着いている時の高さは、実際に一番低くなった所に合わせていく（モデルの立ち姿と少し違っても合うように）
		// 少しずつ上げておき、アニメーションが変わって低い所が変わっても合うようにする
		if (volume > 0.0f) m_footGroundY[f] = std::min(m_footGroundY[f] + kFootGroundRiseSpeed, footY);

		if (footY > m_footGroundY[f] + kFootstepLiftHeight)
		{
			m_isFootLifted[f] = true;
		}
		else if (m_isFootLifted[f] && footY < m_footGroundY[f] + kFootstepPlantHeight)
		{
			// 離れていた足が地面に着いた
			m_isFootLifted[f] = false;
			if (volume > 0.0f && m_footstepCoolTimer <= 0)
			{
				SoundPlayer::PlayFootstep(volume);
				m_footstepCoolTimer = kFootstepCoolFrame;
			}
		}
	}
}

float Player::CalcAnimTravelSpeed(const std::string& animName, Math::Vector3* pOutLocalDir) const
{
	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !it->second || it->second->m_maxLength <= 0.0f) return 0.0f;

	// 最初と最後の腰の位置の差（モデルから見た向き）
	Math::Vector3 move = SampleMotionPos(*it->second, it->second->m_maxLength) - SampleMotionPos(*it->second, 0.0f);
	move.y = 0.0f;
	float length = move.Length();

	// キャラの向きから見た向きに直す（描画で180度回しているので、X・Zを反転する）
	if (pOutLocalDir && length > 0.0f)
	{
		*pOutLocalDir = Math::Vector3(-move.x, 0.0f, -move.z) / length;
	}

	// 1フレーム（再生速度 1.0）あたりに進む距離
	return length / it->second->m_maxLength;
}

void Player::SetupGuardWalkAnims()
{
	m_guardWalkAnims.clear();
	for (const auto& def : kGuardWalkAnimDefs)
	{
		if (!HasAnimation(def.name)) continue;

		GuardWalkAnim anim;
		anim.name = def.name;

		// 名前から決めた向き
		float rad = DirectX::XMConvertToRadians(def.angle);
		anim.localDir = { std::sin(rad), 0.0f, std::cos(rad) };

		// アニメーションの動きから向きと速さを求める（進む動きが入っていれば、そちらを使う）
		Math::Vector3 measuredDir;
		float speed = CalcAnimTravelSpeed(def.name, &measuredDir);
		if (speed * m_animMap[def.name]->m_maxLength >= kMinWalkTravel)
		{
			anim.localDir = measuredDir;
			anim.travelSpeed = speed;
		}
		m_guardWalkAnims.push_back(anim);

		OutputDebugStringA(("ガード歩き: " + anim.name + " 向き(" + std::to_string(anim.localDir.x) + ", "
			+ std::to_string(anim.localDir.z) + ") 速さ " + std::to_string(anim.travelSpeed) + "\n").c_str());
	}

	// 普段の走る速さ（ガード歩きの速さの基準）
	m_runTravelSpeed = CalcAnimTravelSpeed("Combat_Run");

	// ダッシュの全力疾走は、アニメーション自体が普段の走りより速い（その分、ダッシュの倍率から割り引く）
	float dashTravelSpeed = CalcAnimTravelSpeed("Dash_Run");
	m_dashAnimSpeedRatio = (m_runTravelSpeed > 0.0f && dashTravelSpeed > 0.0f) ? dashTravelSpeed / m_runTravelSpeed : 1.0f;
}

float Player::GetGuardMoveSpeed() const
{
	// 普段の走る速さ（アニメーションの速さ × 走る速さの倍率 × モードの倍率）の半分
	float runSpeed = (m_runTravelSpeed > 0.0f) ? m_runTravelSpeed * s_runSpeedRate : kMoveSpeed;
	return runSpeed * GetModeMoveRate() * kGuardMoveRate;
}

const std::string& Player::SelectGuardWalkAnim(const Math::Vector3& moveDir, float moveSpeed, float& outAnimSpeed) const
{
	static const std::string kNone = "";
	outAnimSpeed = 1.0f;
	if (m_guardWalkAnims.empty()) return kNone;

	// 進みたい方向を、キャラの向きから見た向きに直す（前：+Z、右：+X）
	Math::Matrix rot = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), rot);
	Math::Vector3 right = Math::Vector3::TransformNormal(Math::Vector3(1.0f, 0.0f, 0.0f), rot);
	Math::Vector3 local = { moveDir.Dot(right), 0.0f, moveDir.Dot(forward) };

	// 一番近い向きのアニメーションを選ぶ
	const GuardWalkAnim* pBest = &m_guardWalkAnims[0];
	float bestDot = -2.0f;
	for (const auto& anim : m_guardWalkAnims)
	{
		float dot = local.Dot(anim.localDir);
		if (dot > bestDot)
		{
			bestDot = dot;
			pBest = &anim;
		}
	}

	// 移動の速さに合わせて再生速度を変える（足が地面を滑って見えないように）
	if (pBest->travelSpeed > 0.0f)
	{
		outAnimSpeed = std::clamp(moveSpeed / pBest->travelSpeed, kGuardWalkMinAnimSpeed, kGuardWalkMaxAnimSpeed);
	}
	return pBest->name;
}

void Player::TurnTowardCameraForward(float maxAngle)
{
	auto camera = m_camera.lock();
	if (!camera) return;

	Math::Vector3 forward = Math::Vector3::TransformNormal(Math::Vector3(0.0f, 0.0f, 1.0f), camera->GetRotationYMatrix());
	forward.y = 0.0f;
	if (forward.LengthSquared() <= 0.0f) return;
	forward.Normalize();
	TurnToward(forward, maxAngle);
}

float Player::GetTakeoffFrame(const std::string& animName) const
{
	auto it = m_takeoffFrames.find(animName);
	return (it != m_takeoffFrames.end()) ? it->second : 0.0f;
}

float Player::CalcJumpSlamFrame(const std::string& animName) const
{
	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !m_model || !m_model->GetData()) return -1.0f;

	// 剣先の位置（剣のメッシュから求めた刃の形を使う）
	std::vector<const BladeShape*> blades;
	for (const BladeShape* pShape : { &m_bladeShapeR, &m_bladeShapeL })
	{
		if (pShape->nodeIndex >= 0) blades.push_back(pShape);
	}
	if (blades.empty()) return -1.0f;

	// 計算用にモデルを複製し、1フレームずつ再生して、左右の剣先の低い方の高さを記録する
	KdModelWork work(m_model->GetData());
	KdAnimator animator;
	animator.SetAnimation(it->second, false);

	std::vector<float> times;
	std::vector<float> heights;
	for (int i = 0; i < 1000 && !animator.IsAnimationEnd(); ++i)
	{
		times.push_back(animator.GetTime());
		animator.AdvanceTime(work.WorkNodes(), 1.0f);
		work.CalcNodeMatrices();

		float lowest = std::numeric_limits<float>::max();
		for (const BladeShape* pShape : blades)
		{
			Math::Vector3 localTip = pShape->localHand + pShape->localDir * pShape->tipLength;
			Math::Vector3 tip = Math::Vector3::Transform(localTip, work.GetNodes()[pShape->nodeIndex].m_worldTransform);
			lowest = std::min(lowest, tip.y);
		}
		heights.push_back(lowest);
	}
	if (heights.empty()) return -1.0f;

	// ① 一番高く跳んだフレーム（剣先が一番高い所）
	size_t apex = static_cast<size_t>(std::max_element(heights.begin(), heights.end()) - heights.begin());

	// ② そこから後で一番低い高さに、最初に近づいたフレーム（＝落ちてきて地面に叩きつけた瞬間）
	float minAfterApex = *std::min_element(heights.begin() + apex, heights.end());
	for (size_t i = apex; i < heights.size(); ++i)
	{
		if (heights[i] <= minAfterApex + kJumpSlamTolerance) return times[i];
	}
	return -1.0f;
}

float Player::GetSwordTipHeight(int hand) const
{
	// 剣先の、足元からの高さ（最後に描いた剣の位置で調べる。剣の行列は PostUpdate で作るので、1フレーム前の姿勢）
	const Math::Matrix& swordMat = (hand == 1) ? m_swordWorldL : m_swordWorldR;
	return GetSwordPoint(swordMat, m_swordBladeTipY).y - m_pos.y;
}

float Player::GetLandingFrame(const std::string& animName) const
{
	auto it = m_landingFrames.find(animName);
	return (it != m_landingFrames.end()) ? it->second : 0.0f;
}

bool Player::MoveProcess(bool moveByCode)
{
	Math::Vector3 dir;
	if (!GetMoveInputDir(dir)) return false;

	// 旋回（1フレームに最大kMaxTurnAngle度まで）
	TurnToward(dir, kMaxTurnAngle);

	// 座標移動（ルートモーションで動く時は、ここでは動かさない）
	if (moveByCode)
	{
		float moveSpeed = kMoveSpeed * GetModeMoveRate();
		if (IsGuardDashDown()) moveSpeed *= kDashMoveRate;	// ガード・ダッシュのキー（Ctrl ／ RB）を押している間はダッシュ
		m_pos += dir * moveSpeed;
	}
	return true;
}

void Player::TurnToward(const Math::Vector3& dir, float maxAngle)
{
	Math::Matrix nowRot = Math::Matrix::CreateRotationY(GetAngleRad());
	Math::Vector3 nowDir = Math::Vector3::TransformNormal(Math::Vector3(0, 0, 1), nowRot);
	float angle = DirectX::XMConvertToDegrees(acos(std::clamp(nowDir.Dot(dir), -1.0f, 1.0f)));

	if (angle < kMinTurnAngle) return;

	angle = std::min(angle, maxAngle);
	if (nowDir.Cross(dir).y >= 0) { m_angle += angle; }
	else { m_angle -= angle; }
}
