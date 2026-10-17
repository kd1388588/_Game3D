#include "BaseChara.h"
#include "../../../Scene/SceneManager.h"
#include "../../../Utility/GameTime.h"

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
	constexpr float kFootLiftHeight			= 0.03f;	// 立っている高さよりこれ以上上がったら、足が地面から離れたとみなす
	constexpr float kFootNearGroundHeight	= 0.1f;		// 最初のフレームで足がこの高さ以内なら「最初から着地している」とみなす

	// リターゲット
	const std::string kRootNodeName			= "root";
	const std::string kPelvisNodeName		= "pelvis";
	constexpr float kMinPelvisHeight		= 0.0001f;	// これより低い腰の高さは無効とみなす
	constexpr float kSameScaleTolerance		= 0.01f;	// 体格比がこの範囲なら同じ骨格とみなす

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

	// ゲームの時間の速さ（スローモーション中は 1.0 より小さい）
	float timeScale = GameTime::GetScale();

	if (m_model)
	{
		// 1. アニメーション時間を進める
		//    （AdvanceTime は「進める前の時間」でポーズを作るので、その時間を覚えておく）
		float poseTime = m_animator.GetTime();
		ResetPoseToRest();
		m_animator.AdvanceTime(m_model->WorkNodes(), m_animSpeed * timeScale);

		// 2. ルートモーションの移動量を計算
		if (m_extractPelvisMotion)
		{
			// 腰の水平移動をキャラの移動として取り出す
			UpdatePelvisMotion(poseTime);
		}
		else if (KdModelWork::Node* pRoot = FindRootNode(m_model->WorkNodes()))
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

	// 落下中（m_gravity が正）だけ、重力の倍率をかける
	// （地面にいる時は毎フレーム 0 に戻るので、接地の判定には影響しない）
	// スローモーション中は、重力も落ちる速さもゆっくりにする
	float accel = (m_gravity > 0.0f) ? kGravityAccel * m_fallGravityRate : kGravityAccel;
	m_gravity += accel * timeScale;
	m_pos.y -= m_gravity * timeScale;
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
			// アニメーションのファイルとモデルで骨の並びや長さが違っても動くように合わせる
			if (m_model && m_model->GetData())
			{
				anim = RetargetAnimation(anim, *spModelData, *m_model->GetData(), info.name);
			}
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

bool BaseChara::SampleNodeHeights(const std::string& animName, const std::vector<std::string>& nodeNames,
	std::vector<float>& outTimes, std::vector<std::vector<float>>& outHeights) const
{
	outTimes.clear();
	outHeights.clear();

	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !m_model || !m_model->GetData()) return false;

	// 計算用にモデルを複製（表示中のモデルには影響させない）
	KdModelWork work(m_model->GetData());

	std::vector<const KdModelWork::Node*> nodes;
	for (const auto& name : nodeNames)
	{
		if (const KdModelWork::Node* pNode = work.FindNode(name))
		{
			nodes.push_back(pNode);
		}
	}
	if (nodes.empty())
	{
		OutputDebugStringA(("【足の骨が見つかりません】: " + animName + "\n").c_str());
		return false;
	}

	KdAnimator animator;
	animator.SetAnimation(it->second, false);

	// 1フレームずつ再生して、各フレームの骨の高さを記録する
	for (int i = 0; i < kMaxSampleFrames && !animator.IsAnimationEnd(); ++i)
	{
		outTimes.push_back(animator.GetTime());
		animator.AdvanceTime(work.WorkNodes(), 1.0f);

		// ゲーム中と同じくルートの移動を打ち消してから計算する
		if (KdModelWork::Node* pRoot = FindRootNode(work.WorkNodes()))
		{
			pRoot->m_localTransform.Translation(Math::Vector3::Zero);
		}
		work.CalcNodeMatrices();

		std::vector<float> heights;
		for (const KdModelWork::Node* pNode : nodes)
		{
			heights.push_back(pNode->m_worldTransform.Translation().y);
		}
		outHeights.push_back(heights);
	}

	return !outTimes.empty();
}

float BaseChara::CalcFootPlantFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const
{
	std::vector<float> frameTimes;
	std::vector<std::vector<float>> footHeights;
	if (!SampleNodeHeights(animName, footNodeNames, frameTimes, footHeights)) return 0.0f;

	size_t footNum = footHeights[0].size();

	// 足ごとの一番低い位置（＝地面についている高さ）
	std::vector<float> minHeights = footHeights[0];
	for (const auto& heights : footHeights)
	{
		for (size_t f = 0; f < footNum; ++f)
		{
			minHeights[f] = std::min(minHeights[f], heights[f]);
		}
	}

	// 最初のフレームで、すべての足が地面の近くにあれば「着地した瞬間から始まるアニメーション」とみなし、0 を返す
	// （つま先→かかとの順に着く着地では、かかとが着く（一番低くなる）のが少し後のフレームになる。
	//   そのフレームを着地とすると、着地のアニメーションを空中で早めに始めてしまい、
	//   空中でひざを曲げて「つぶれる」ように見えてしまうため）
	bool isStartOnGround = true;
	for (size_t f = 0; f < footNum; ++f)
	{
		if (footHeights[0][f] > minHeights[f] + kFootNearGroundHeight)
		{
			isStartOnGround = false;
			break;
		}
	}
	if (isStartOnGround) return 0.0f;

	// すべての足が地面についている最初のフレームを探す
	for (size_t i = 0; i < frameTimes.size(); ++i)
	{
		bool isAllPlanted = true;
		for (size_t f = 0; f < footNum; ++f)
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

float BaseChara::CalcFootLiftFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const
{
	std::vector<float> frameTimes;
	std::vector<std::vector<float>> footHeights;
	if (!SampleNodeHeights(animName, footNodeNames, frameTimes, footHeights)) return -1.0f;

	size_t footNum = footHeights[0].size();

	// 最初のフレームの足の高さ（＝地面に立っている高さ）より、すべての足が上がった最初のフレームを探す
	for (size_t i = 0; i < frameTimes.size(); ++i)
	{
		bool isAllLifted = true;
		for (size_t f = 0; f < footNum; ++f)
		{
			if (footHeights[i][f] <= footHeights[0][f] + kFootLiftHeight)
			{
				isAllLifted = false;
				break;
			}
		}

		if (isAllLifted) return frameTimes[i];
	}

	return -1.0f;
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

std::shared_ptr<KdAnimationData> BaseChara::RetargetAnimation(const std::shared_ptr<KdAnimationData>& srcAnim,
	const KdModelData& srcModel, const KdModelData& dstModel, const std::string& debugName)
{
	const auto& srcNodes = srcModel.GetOriginalNodes();
	const auto& dstNodes = dstModel.GetOriginalNodes();

	// モデル側の骨を名前で引けるようにする
	std::unordered_map<std::string, int> dstIndexMap;
	for (int i = 0; i < static_cast<int>(dstNodes.size()); ++i)
	{
		dstIndexMap[dstNodes[i].m_name] = i;
	}

	// 体格の比率（腰の高さで比べる）
	auto getPelvisHeight = [](const std::vector<KdModelData::Node>& nodes)
	{
		for (const auto& node : nodes)
		{
			if (node.m_name == kPelvisNodeName) return node.m_worldTransform.Translation().y;
		}
		return 0.0f;
	};
	float srcPelvisHeight = getPelvisHeight(srcNodes);
	float dstPelvisHeight = getPelvisHeight(dstNodes);
	float heightRatio = (srcPelvisHeight > kMinPelvisHeight && dstPelvisHeight > kMinPelvisHeight)
		? dstPelvisHeight / srcPelvisHeight : 1.0f;

	// 骨の並びと長さが元から一致していれば、そのまま使う
	bool isSameSkeleton = (std::abs(heightRatio - 1.0f) < kSameScaleTolerance);
	for (const auto& node : srcAnim->m_nodes)
	{
		if (!isSameSkeleton) break;

		int offset = node.m_nodeOffset;
		isSameSkeleton = offset >= 0 && offset < static_cast<int>(srcNodes.size()) && offset < static_cast<int>(dstNodes.size())
			&& srcNodes[offset].m_name == dstNodes[offset].m_name;
	}
	if (isSameSkeleton) return srcAnim;

	// 骨を名前で対応づけて作り直す
	auto dstAnim = std::make_shared<KdAnimationData>();
	dstAnim->m_name = srcAnim->m_name;
	dstAnim->m_maxLength = srcAnim->m_maxLength;

	int skipCount = 0;
	for (const auto& srcNode : srcAnim->m_nodes)
	{
		int srcOffset = srcNode.m_nodeOffset;
		if (srcOffset < 0 || srcOffset >= static_cast<int>(srcNodes.size())) { skipCount++; continue; }

		const std::string& boneName = srcNodes[srcOffset].m_name;
		auto it = dstIndexMap.find(boneName);
		if (it == dstIndexMap.end()) { skipCount++; continue; }

		KdAnimationData::Node dstNode = srcNode;
		dstNode.m_nodeOffset = it->second;

		if (boneName == kRootNodeName || boneName == kPelvisNodeName)
		{
			// 体の移動（ルートモーション・腰の上下）は体格に合わせて拡大縮小
			for (auto& key : dstNode.m_translations)
			{
				key.m_vec *= heightRatio;
			}
		}
		else
		{
			// それ以外の骨は、骨の長さ（位置）をモデル側のものに固定する
			dstNode.m_translations.clear();
			dstNode.m_translations.push_back({ 0.0f, dstNodes[it->second].m_localTransform.Translation() });

			// 拡縮はモデルの単位とずれることがあるので使わない
			dstNode.m_scales.clear();
		}

		dstAnim->m_nodes.push_back(dstNode);
	}

	char log[256];
	sprintf_s(log, "【リターゲット】%s : 体格比 %.3f / 対応しない骨 %d 本\n", debugName.c_str(), heightRatio, skipCount);
	OutputDebugStringA(log);

	return dstAnim;
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

		// Update と同じく、落下中は重力の倍率をかける
		gravity += (gravity > 0.0f) ? kGravityAccel * m_fallGravityRate : kGravityAccel;
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

void BaseChara::FindMotionNodes()
{
	m_isMotionNodeReady = true;
	if (!m_model || !m_model->GetData()) return;

	const auto& nodes = m_model->GetData()->GetOriginalNodes();
	for (int i = 0; i < static_cast<int>(nodes.size()); ++i)
	{
		if (nodes[i].m_name == kRootNodeName)
		{
			m_rootNodeIndex = i;
		}
		else if (nodes[i].m_name == kPelvisNodeName)
		{
			m_pelvisNodeIndex = i;
			m_pelvisRestPos = nodes[i].m_localTransform.Translation();
		}
	}
}

void BaseChara::ResetPoseToRest()
{
	if (!m_model || !m_model->GetData()) return;

	const auto& restNodes = m_model->GetData()->GetOriginalNodes();
	auto& workNodes = m_model->WorkNodes();
	size_t count = std::min(restNodes.size(), workNodes.size());
	for (size_t i = 0; i < count; ++i)
	{
		workNodes[i].m_localTransform = restNodes[i].m_localTransform;
	}
}

bool BaseChara::HasHorizontalMotion(const std::string& animName) const
{
	constexpr float kMinTravelDistance = 0.05f;	// 1回の再生でこれ以上進めば「前へ進む動きがある」とみなす

	auto it = m_animMap.find(animName);
	if (it == m_animMap.end() || !it->second || m_pelvisNodeIndex < 0) return false;

	// 最初と最後の腰の位置を比べる
	Math::Vector3 start = SampleMotionPos(*it->second, 0.0f);
	Math::Vector3 end = SampleMotionPos(*it->second, it->second->m_maxLength);
	return (end - start).Length() >= kMinTravelDistance;
}

Math::Vector3 BaseChara::SampleMotionPos(const KdAnimationData& anim, float time, Math::Vector3* pOutPelvisPos) const
{
	Math::Vector3 rootPos = Math::Vector3::Zero;
	Math::Vector3 pelvisPos = m_pelvisRestPos;

	// アニメーションから、root と pelvis の位置だけを取り出す
	for (const auto& animNode : anim.m_nodes)
	{
		if (animNode.m_nodeOffset == m_rootNodeIndex)
		{
			animNode.InterpolateTranslations(rootPos, time);
		}
		else if (animNode.m_nodeOffset == m_pelvisNodeIndex)
		{
			animNode.InterpolateTranslations(pelvisPos, time);
		}
	}

	if (pOutPelvisPos) *pOutPelvisPos = pelvisPos;

	// root の移動 ＋ 腰が立ち位置からずれた分（上下は除く）
	return
	{
		rootPos.x + (pelvisPos.x - m_pelvisRestPos.x),
		0.0f,
		rootPos.z + (pelvisPos.z - m_pelvisRestPos.z),
	};
}

void BaseChara::UpdatePelvisMotion(float poseTime)
{
	if (!m_isMotionNodeReady) FindMotionNodes();
	if (m_pelvisNodeIndex < 0 || !m_currentAnim) return;

	// ① 今回のポーズの腰の水平位置
	Math::Vector3 animPelvisPos;
	Math::Vector3 motionPos = SampleMotionPos(*m_currentAnim, poseTime, &animPelvisPos);

	// ② 前回からの移動量
	Math::Vector3 delta = Math::Vector3::Zero;
	if (m_prevMotionTime < 0.0f)
	{
		// アニメーションを切り替えた直後は、移動の基準を作るだけ
	}
	else if (poseTime < m_prevMotionTime)
	{
		// ループして最初に戻った：最後までの移動 ＋ 最初からの移動
		delta = (SampleMotionPos(*m_currentAnim, m_currentAnim->m_maxLength) - m_prevMotionPos)
			+ (motionPos - SampleMotionPos(*m_currentAnim, 0.0f));
	}
	else
	{
		delta = motionPos - m_prevMotionPos;
	}

	m_rootMoveDelta = delta;
	m_prevMotionPos = motionPos;
	m_prevMotionTime = poseTime;

	// ③ root の移動は、今までどおり見た目からは消す
	auto& nodes = m_model->WorkNodes();
	if (m_rootNodeIndex >= 0)
	{
		nodes[m_rootNodeIndex].m_localTransform.Translation(Math::Vector3::Zero);
	}

	// ④ 腰の水平位置を決める（高さはアニメーションのまま）
	//    ・ルートモーションを使う時：腰をキャラの位置に固定する（動いた分はキャラ自体を動かす）
	//    ・使わない時（移動を止めている攻撃・待機・回避など）：今のアニメーションの位置そのまま
	//      （固定すると、回転する攻撃などで剣を振る位置が変わってしまうため）
	//    どちらも「前のアニメーションと混ぜた位置」は使わない
	//    （前のアニメーションで進んだ分はキャラの位置に入っているので、混ぜると体が一瞬前にずれる）
	Math::Vector3 pelvisPos = nodes[m_pelvisNodeIndex].m_localTransform.Translation();
	if (m_useRootMotion)
	{
		pelvisPos.x = m_pelvisRestPos.x;
		pelvisPos.z = m_pelvisRestPos.z;
	}
	else
	{
		pelvisPos.x = animPelvisPos.x;
		pelvisPos.z = animPelvisPos.z;
	}

	// ⑤ 空中では、アニメーションで腰が上がる分を止める（高さはプログラムのジャンプに任せる）
	if (m_pelvisMaxY >= 0.0f)
	{
		pelvisPos.y = std::min(pelvisPos.y, m_pelvisMaxY);
	}
	nodes[m_pelvisNodeIndex].m_localTransform.Translation(pelvisPos);
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
		m_currentAnim = targetAnim;
		m_animator.SetAnimation(targetAnim, isLoop, blendFrame);

		m_prevAnimRootPos = Math::Vector3::Zero;
		m_prevMotionTime = -1.0f;	// 腰の移動の基準を作り直す
	}
}
