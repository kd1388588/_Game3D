#include "BaseChara.h"
#include "../../../Scene/SceneManager.h"

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
			if (node.m_name == "root")
			{
				// 今のフレームの絶対座標を取得
				Math::Vector3 currentAnimRootPos = node.m_localTransform.Translation();

				Math::Vector3 delta = currentAnimRootPos - m_prevAnimRootPos;

				// アニメーション開始直後や、ブレンドで原点に巻き戻る時の「逆走・ワープ」を防ぐ
				if (m_animator.GetTime() <= m_animSpeed * 2.0f || delta.LengthSquared() > 10000.0f)
				{
					delta = Math::Vector3::Zero;
				}

				m_rootMoveDelta = delta;
				m_prevAnimRootPos = currentAnimRootPos; 

				// 最後にモデルが原点からズレないようにXZYを「0」にリセット
				currentAnimRootPos.x = 0.0f;
				currentAnimRootPos.y = 0.0f; 
				currentAnimRootPos.z = 0.0f;
				node.m_localTransform.Translation(currentAnimRootPos);

				break;
			}
		}
		m_model->CalcNodeMatrices();
	}

	m_gravity += 0.01f;     
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
			std::string errorMsg = "【アニメーション読込失敗 (ファイルなし)】: " + info.path + "\n";
			OutputDebugStringA(errorMsg.c_str());
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
			std::string errorMsg = "【アニメーションデータなし】: " + info.path + "\n";
			OutputDebugStringA(errorMsg.c_str());
		}
	}
}

void BaseChara::GroundHit()
{
	if (m_gravity < 0.0f) return;

	KdCollider::RayInfo ray;
	ray.m_pos = m_pos;
	ray.m_dir = { 0, -1, 0 };
	ray.m_pos.y += 0.3f;

	float enableStepHigh = 0.2f;
	ray.m_range = m_gravity + enableStepHigh + 0.3f;
	ray.m_type = KdCollider::TypeGround;

	std::list<KdCollider::CollisionResult> retRayList;

	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() != this)
		{
			obj->Intersects(ray, &retRayList);
		}
	}

	float maxOverLap = 0.0f;
	Math::Vector3 hitPos;
	bool isHit = false;

	for (auto& ret : retRayList)
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
	center.y += 0.7f;

	KdCollider::SphereInfo sphere(KdCollider::TypeBump, center, 0.3f);

	std::list<KdCollider::CollisionResult> retList;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() != this)
		{
			obj->Intersects(sphere, &retList);
		}
	}

	for (auto& ret : retList)
	{
		if (ret.m_hitDir.y > 0.3f)
		{
			continue;
		}

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

void BaseChara::ChangeAnimation(const std::string& animName, bool isLoop, bool forceRestart, float blendFrame)
{
	if (!forceRestart && m_currentAnimName == animName && !m_animator.IsAnimationEnd())
	{
		return;
	}

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