#include "BaseChara.h"
#include "../../../Scene/SceneManager.h"

namespace
{
	constexpr float kGravityAccel			= 0.01f;	// 1フレームあたりの重力加速度
	constexpr float kRootWarpThresholdSq	= 10000.0f;	// これ以上のルート移動はワープとみなして無視する

	// 地面判定
	constexpr float kGroundRayStartOffset	= 0.3f;		// レイの発射位置を足元からどれだけ上げるか
	constexpr float kEnableStepHeight		= 0.2f;		// 乗り越えられる段差の高さ
	constexpr float kGroundSearchRange		= 30.0f;	// 着地予測で地面を探す距離

	// 足の接地判定
	constexpr int	kMaxSampleFrames		= 1000;		// アニメーションを調べる最大フレーム数
	constexpr float kFootPlantTolerance		= 0.03f;	// 最も低い位置からこの高さ以内なら接地とみなす

	// 押し出し判定
	constexpr float kBumpCenterHeight		= 0.7f;
	constexpr float kBumpRadius				= 0.3f;
	constexpr float kBumpIgnoreHitDirY		= 0.3f;		// これより上向きの衝突は床とみなして押し出さない
}

template<class ShapeInfo>
std::list<KdCollider::CollisionResult> BaseChara::IntersectsOthers(const ShapeInfo& shape) const
{
	std::list<KdCollider::CollisionResult> results;
	for (auto& obj : SceneManager::Instance().GetObjList())
	{
		if (obj.get() != this)
		{
			obj->Intersects(shape, &results);
		}
	}
	return results;
}

KdModelWork::Node* BaseChara::FindRootNode(std::vector<KdModelWork::Node>& nodes)
{
	for (auto& node : nodes)
	{
		if (node.m_name == "root") return &node;
	}
	return nullptr;
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
		if (KdModelWork::Node* pRoot = FindRootNode(m_model->WorkNodes()))
		{
			// 今のフレームの絶対座標を取得
			Math::Vector3 currentAnimRootPos = pRoot->m_localTransform.Translation();
			Math::Vector3 delta = currentAnimRootPos - m_prevAnimRootPos;

			// アニメーション開始直後や、ブレンドで原点に巻き戻る時の「逆走・ワープ」を防ぐ
			if (m_animator.GetTime() <= m_animSpeed * 2.0f || delta.LengthSquared() > kRootWarpThresholdSq)
			{
				delta = Math::Vector3::Zero;
			}

			m_rootMoveDelta = delta;
			m_prevAnimRootPos = currentAnimRootPos;

			// 最後にモデルが原点からズレないようにリセット
			pRoot->m_localTransform.Translation(Math::Vector3::Zero);
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

bool BaseChara::HasAnimation(const std::string& animName) const
{
	if (m_animMap.find(animName) != m_animMap.end()) return true;
	return m_model && m_model->GetAnimation(animName);
}

float BaseChara::CalcFootPlantFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const
{
	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !m_model || !m_model->GetData()) return 0.0f;

	// 計算用にモデルを複製（表示中のモデルには影響させない）
	KdModelWork work(m_model->GetData());

	std::vector<const KdModelWork::Node*> feet;
	for (const auto& name : footNodeNames)
	{
		if (const KdModelWork::Node* pNode = work.FindNode(name))
		{
			feet.push_back(pNode);
		}
	}
	if (feet.empty())
	{
		OutputDebugStringA(("【足の骨が見つかりません】: " + animName + "\n").c_str());
		return 0.0f;
	}

	KdAnimator animator;
	animator.SetAnimation(it->second, false);

	// 1フレームずつ再生して、各フレームの足の高さを記録する
	std::vector<float> frameTimes;
	std::vector<std::vector<float>> footHeights;

	for (int i = 0; i < kMaxSampleFrames && !animator.IsAnimationEnd(); ++i)
	{
		frameTimes.push_back(animator.GetTime());
		animator.AdvanceTime(work.WorkNodes(), 1.0f);

		// ゲーム中と同じくルートの移動を打ち消してから計算する
		if (KdModelWork::Node* pRoot = FindRootNode(work.WorkNodes()))
		{
			pRoot->m_localTransform.Translation(Math::Vector3::Zero);
		}
		work.CalcNodeMatrices();

		std::vector<float> heights;
		for (const KdModelWork::Node* pFoot : feet)
		{
			heights.push_back(pFoot->m_worldTransform.Translation().y);
		}
		footHeights.push_back(heights);
	}

	if (frameTimes.empty()) return 0.0f;

	// 足ごとの一番低い位置（＝地面についている高さ）
	std::vector<float> minHeights = footHeights[0];
	for (const auto& heights : footHeights)
	{
		for (size_t f = 0; f < feet.size(); ++f)
		{
			minHeights[f] = std::min(minHeights[f], heights[f]);
		}
	}

	// すべての足が地面についている最初のフレームを探す
	for (size_t i = 0; i < frameTimes.size(); ++i)
	{
		bool isAllPlanted = true;
		for (size_t f = 0; f < feet.size(); ++f)
		{
			if (footHeights[i][f] > minHeights[f] + kFootPlantTolerance)
			{
				isAllPlanted = false;
				break;
			}
		}

		if (isAllPlanted) return frameTimes[i];
	}

	return 0.0f;
}

bool BaseChara::FindGroundBelow(float range, Math::Vector3& outHitPos) const
{
	KdCollider::RayInfo ray;
	ray.m_pos = m_pos;
	ray.m_pos.y += kGroundRayStartOffset;
	ray.m_dir = { 0, -1, 0 };
	ray.m_range = range + kGroundRayStartOffset;
	ray.m_type = KdCollider::TypeGround;

	// 一番深くめり込んでいる地点を探す
	float maxOverLap = 0.0f;
	bool isHit = false;

	for (auto& ret : IntersectsOthers(ray))
	{
		if (maxOverLap < ret.m_overlapDistance)
		{
			maxOverLap = ret.m_overlapDistance;
			outHitPos = ret.m_hitPos;
			isHit = true;
		}
	}

	return isHit;
}

void BaseChara::GroundHit()
{
	// 上昇中は判定しない
	if (m_gravity < 0.0f) return;

	Math::Vector3 hitPos;
	if (FindGroundBelow(m_gravity + kEnableStepHeight, hitPos))
	{
		m_pos.y = hitPos.y;
		m_gravity = 0.0f;
	}
}

int BaseChara::PredictLandingFrames(int maxFrames) const
{
	// 上昇中は予測しない
	if (m_gravity < 0.0f) return -1;

	Math::Vector3 hitPos;
	if (!FindGroundBelow(kGroundSearchRange, hitPos)) return -1;

	// GroundHitと同じ条件（落下量＋段差の高さ以内に地面がある）で着地を判定しながら落下をシミュレートする
	float height = m_pos.y - hitPos.y;
	float gravity = m_gravity;

	for (int frame = 0; frame <= maxFrames; ++frame)
	{
		if (height <= gravity + kEnableStepHeight) return frame;

		gravity += kGravityAccel;
		height -= gravity;
	}

	return -1;
}

void BaseChara::BumpHit()
{
	Math::Vector3 center = m_pos;
	center.y += kBumpCenterHeight;

	KdCollider::SphereInfo sphere(KdCollider::TypeBump, center, kBumpRadius);

	for (auto& ret : IntersectsOthers(sphere))
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
