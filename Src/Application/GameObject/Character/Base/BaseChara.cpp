#include "BaseChara.h"
#include "../../../Scene/SceneManager.h"

namespace
{
	constexpr float kGravityAccel			= 0.01f;	// 1フレームあたりの重力加速度
	constexpr float kRootWarpThresholdSq	= 10000.0f;	// これ以上のルート移動はワープとみなして無視する

	// 地面判定
	constexpr float kGroundRayStartOffset	= 0.3f;		// レイの発射位置を足元からどれだけ上げるか
	constexpr float kEnableStepHeight		= 0.2f;		// 乗り越えられる段差の高さ

	// 押し出し判定
	constexpr float kBumpCenterHeight		= 0.7f;
	constexpr float kBumpRadius				= 0.3f;
	constexpr float kBumpIgnoreHitDirY		= 0.3f;		// これより上向きの衝突は床とみなして押し出さない

	// 自分以外の全オブジェクトと当たり判定を行い、結果をまとめて返す
	template<class ShapeInfo>
	std::list<KdCollider::CollisionResult> IntersectsOthers(const KdGameObject* self, const ShapeInfo& shape)
	{
		std::list<KdCollider::CollisionResult> results;
		for (auto& obj : SceneManager::Instance().GetObjList())
		{
			if (obj.get() != self)
			{
				obj->Intersects(shape, &results);
			}
		}
		return results;
	}
}

void BaseChara::PostUpdate()
{
	GroundHit();
	BumpHit();
}

void BaseChara::Update()
{
	m_rootMoveDelta = Math::Vector3::Zero;

	if (m_model)
	{
		// 1. アニメーション時間を進める
		m_animator.AdvanceTime(m_model->WorkNodes(), m_animSpeed);

		// 2. ルートモーションの移動量を計算
		for (auto& node : m_model->WorkNodes())
		{
			if (node.m_name != "root") continue;

			// 今のフレームの絶対座標を取得
			Math::Vector3 currentAnimRootPos = node.m_localTransform.Translation();
			Math::Vector3 delta = currentAnimRootPos - m_prevAnimRootPos;

			// アニメーション開始直後や、ブレンドで原点に巻き戻る時の「逆走・ワープ」を防ぐ
			if (m_animator.GetTime() <= m_animSpeed * 2.0f || delta.LengthSquared() > kRootWarpThresholdSq)
			{
				delta = Math::Vector3::Zero;
			}

			m_rootMoveDelta = delta;
			m_prevAnimRootPos = currentAnimRootPos;

			// 最後にモデルが原点からズレないようにリセット
			node.m_localTransform.Translation(Math::Vector3::Zero);
			break;
		}
		m_model->CalcNodeMatrices();
	}

	m_gravity += kGravityAccel;
	m_pos.y -= m_gravity;
}

void BaseChara::DrawLit()
{
}

// アニメーションデータをマップに登録
void BaseChara::SetAnimationData(const std::string& name, const std::shared_ptr<KdAnimationData>& animData)
{
	if (animData)
	{
		m_animMap[name] = animData;
	}
}

void BaseChara::LoadAnimations(const std::vector<AnimLoadInfo>& loadList)
{
	for (const auto& info : loadList)
	{
		// 1. ファイルから直接モデルデータを取得
		auto spModelData = KdAssets::Instance().m_modeldatas.GetData(info.path);

		// 2. ファイルが存在しない場合はクラッシュさせずに警告ログを出してスキップ
		if (!spModelData)
		{
			OutputDebugStringA(("【アニメーション読込失敗 (ファイルなし)】: " + info.path + "\n").c_str());
			continue;
		}

		// 3. 正常に取得できた場合のみKdModelWorkにセットしてアニメーションを取り出す
		KdModelWork animModel;
		animModel.SetModelData(spModelData);

		if (auto anim = animModel.GetAnimation(0))
		{
			SetAnimationData(info.name, anim);
		}
		else
		{
			OutputDebugStringA(("【アニメーションデータなし】: " + info.path + "\n").c_str());
		}
	}
}

void BaseChara::GroundHit()
{
	// 上昇中は判定しない
	if (m_gravity < 0.0f) return;

	KdCollider::RayInfo ray;
	ray.m_pos = m_pos;
	ray.m_pos.y += kGroundRayStartOffset;
	ray.m_dir = { 0, -1, 0 };
	ray.m_range = m_gravity + kEnableStepHeight + kGroundRayStartOffset;
	ray.m_type = KdCollider::TypeGround;

	// 一番深くめり込んでいる地点を探す
	float maxOverLap = 0.0f;
	Math::Vector3 hitPos;
	bool isHit = false;

	for (auto& ret : IntersectsOthers(this, ray))
	{
		if (maxOverLap < ret.m_overlapDistance)
		{
			maxOverLap = ret.m_overlapDistance;
			hitPos = ret.m_hitPos;
			isHit = true;
		}
	}

	if (isHit)
	{
		m_pos.y = hitPos.y;
		m_gravity = 0.0f;
	}
}

void BaseChara::BumpHit()
{
	Math::Vector3 center = m_pos;
	center.y += kBumpCenterHeight;

	KdCollider::SphereInfo sphere(KdCollider::TypeBump, center, kBumpRadius);

	for (auto& ret : IntersectsOthers(this, sphere))
	{
		// 床のような上向きの衝突は無視
		if (ret.m_hitDir.y > kBumpIgnoreHitDirY) continue;

		// 水平方向にだけ押し出す
		Math::Vector3 pushDir = ret.m_hitDir;
		pushDir.y = 0.0f;
		pushDir.Normalize();

		m_pos += pushDir * ret.m_overlapDistance;
	}
}

void BaseChara::Release()
{
	m_model = nullptr;
}

void BaseChara::UpdateInvincibleTimer()
{
	if (m_invincibleTimer > 0)
	{
		m_invincibleTimer--;
	}
}

void BaseChara::ApplyRootMotion(float scale, float rotYRad)
{
	if (!m_useRootMotion || m_rootMoveDelta.LengthSquared() <= 0.0f) return;

	Math::Vector3 fixedDelta = m_rootMoveDelta;
	fixedDelta.z *= -1.0f;	// Z軸の向き補正
	fixedDelta *= scale;

	Math::Matrix rotY = Math::Matrix::CreateRotationY(rotYRad);
	m_pos += Math::Vector3::TransformNormal(fixedDelta, rotY);
}

void BaseChara::ChangeAnimation(const std::string& animName, bool isLoop, bool forceRestart, float blendFrame)
{
	// 同じアニメーションを再生中なら何もしない
	if (!forceRestart && m_currentAnimName == animName && !m_animator.IsAnimationEnd())
	{
		return;
	}

	// 登録済みのアニメーションを優先し、なければモデル内蔵のものを探す
	std::shared_ptr<KdAnimationData> targetAnim = nullptr;
	auto it = m_animMap.find(animName);
	if (it != m_animMap.end()) { targetAnim = it->second; }
	else if (m_model) { targetAnim = m_model->GetAnimation(animName); }

	if (targetAnim)
	{
		m_currentAnimName = animName;
		m_animator.SetAnimation(targetAnim, isLoop, blendFrame);

		m_prevAnimRootPos = Math::Vector3::Zero;
	}
}
