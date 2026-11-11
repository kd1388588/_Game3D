#include "KdAnimation.h"
#include "../Direct3D/KdModel.h"

// 二分探索で、指定時間から次の配列要素のKeyIndexを求める関数
// list		… キー配列
// time		… 時間
// 戻り値	… 次の配列要素のIndex
template<class T>
int BinarySearchNextAnimKey(const std::vector<T>& list, float time)
{
	int low = 0;
	int high = (int)list.size();
	while (low < high)
	{
		int mid = (low + high) / 2;
		float midTime = list[mid].m_time;

		if (midTime <= time) low = mid + 1;
		else high = mid;
	}
	return low;
}

bool KdAnimationData::Node::InterpolateTranslations(Math::Vector3& result, float time)
{
	if (m_translations.size() == 0)return false;

	// キー位置検索
	UINT keyIdx = BinarySearchNextAnimKey(m_translations, time);

	// 先頭のキーなら、先頭のデータを返す
	if (keyIdx == 0) {
		result = m_translations.front().m_vec;
		return true;
	}
	// 配列外のキーなら、最後のデータを返す
	else if (keyIdx >= m_translations.size()) {
		result = m_translations.back().m_vec;
		return true;
	}
	// それ以外(中間の時間)なら、その時間の値を補間計算で求める
	else {
		auto& prev = m_translations[keyIdx - 1];	// 前のキー
		auto& next = m_translations[keyIdx];		// 次のキー
		// 前のキーと次のキーの時間から、0～1間の時間を求める
		float f = (time - prev.m_time) / (next.m_time - prev.m_time);
		// 補間
		result = DirectX::XMVectorLerp(
			prev.m_vec,
			next.m_vec,
			f
		);
	}

	return true;
}

bool KdAnimationData::Node::InterpolateRotations(Math::Quaternion& result, float time)
{
	if (m_rotations.size() == 0)return false;

	// キー位置検索
	UINT keyIdx = BinarySearchNextAnimKey(m_rotations, time);
	// 先頭のキーなら、先頭のデータを返す
	if (keyIdx == 0) {
		result = m_rotations.front().m_quat;
	}
	// 配列外のキーなら、最後のデータを返す
	else if (keyIdx >= m_rotations.size()) {
		result = m_rotations.back().m_quat;
	}
	// それ以外(中間の時間)なら、その時間の値を補間計算で求める
	else {
		auto& prev = m_rotations[keyIdx - 1];	// 前のキー
		auto& next = m_rotations[keyIdx];		// 次のキー
		// 前のキーと次のキーの時間から、0～1間の時間を求める
		float f = (time - prev.m_time) / (next.m_time - prev.m_time);
		// 補間
		result = DirectX::XMQuaternionSlerp(
			prev.m_quat,
			next.m_quat,
			f
		);
	}

	return true;
}

bool KdAnimationData::Node::InterpolateScales(Math::Vector3& result, float time)
{
	if (m_scales.size() == 0)return false;

	// キー位置検索
	UINT keyIdx = BinarySearchNextAnimKey(m_scales, time);

	// 先頭のキーなら、先頭のデータを返す
	if (keyIdx == 0) {
		result = m_scales.front().m_vec;
		return true;
	}
	// 配列外のキーなら、最後のデータを返す
	else if (keyIdx >= m_scales.size()) {
		result = m_scales.back().m_vec;
		return true;
	}
	// それ以外(中間の時間)なら、その時間の値を補間計算で求める
	else {
		auto& prev = m_scales[keyIdx - 1];	// 前のキー
		auto& next = m_scales[keyIdx];		// 次のキー
		// 前のキーと次のキーの時間から、0～1間の時間を求める
		float f = (time - prev.m_time) / (next.m_time - prev.m_time);
		// 補間
		result = DirectX::XMVectorLerp(
			prev.m_vec,
			next.m_vec,
			f
		);
	}

	return true;
}

void KdAnimationData::Node::Interpolate(Math::Matrix& rDst, float time)
{
	// ベクターによる拡縮補間
	bool isChange = false;
	Math::Matrix scale;
	Math::Vector3 resultVec;
	if (InterpolateScales(resultVec, time))
	{
		scale = scale.CreateScale(resultVec);
		isChange = true;
	}

	// クォタニオンによる回転補間
	Math::Matrix rotate;
	Math::Quaternion resultQuat;
	if (InterpolateRotations(resultQuat, time))
	{
		rotate = rotate.CreateFromQuaternion(resultQuat);
		isChange = true;
	}

	// ベクターによる座標補間
	Math::Matrix trans;
	if (InterpolateTranslations(resultVec, time))
	{
		trans = trans.CreateTranslation(resultVec);
		isChange = true;
	}

	if (isChange)
	{
		rDst = scale * rotate * trans;
	}
}

//void KdAnimator::AdvanceTime(std::vector<KdModelWork::Node>& rNodes, float speed)
//{
//	if (!m_spAnimation) { return; }
//
//	// 全てのアニメーションノード（モデルの行列を補間する情報）の行列補間を実行する
//	for (auto& rAnimNode : m_spAnimation->m_nodes)
//	{
//		// 対応するモデルノードのインデックス
//		UINT idx = rAnimNode.m_nodeOffset;
//
//		auto prev = rNodes[idx].m_localTransform;
//
//		// アニメーションデータによる行列補間
//		rAnimNode.Interpolate(rNodes[idx].m_localTransform, m_time);
//
//		prev = rNodes[idx].m_localTransform;
//	}
//
//	// アニメーションのフレームを進める
//	m_time += speed;
//
//	// アニメーションデータの最後のフレームを超えたら
//	if (m_time >= m_spAnimation->m_maxLength)
//	{
//		if (m_isLoop)
//		{
//			// アニメーションの最初に戻る（ループさせる
//			m_time = 0.0f;
//		}
//		else
//		{
//			m_time = m_spAnimation->m_maxLength;
//		}
//	}
//}


//////////////////////////////////////////////////////////////////////////////////////////
// 0915 攻撃アニメーションの制御のため追加 戻す際は上のプログラムのコメントを外す
//////////////////////////////////////////////////////////////////////////////////////////
void KdAnimator::AdvanceTime(std::vector<KdModelWork::Node>& rNodes, float speed)
{
	if (!m_spAnimation) { return; }

	// ① ブレンドの進行度を計算
	bool isBlending = false;
	float blendRatio = 1.0f;

	if (m_spPrevAnimation && m_blendDuration > 0.0f)
	{
		m_currentBlendTimer += speed;
		blendRatio = m_currentBlendTimer / m_blendDuration;

		if (blendRatio >= 1.0f)
		{
			blendRatio = 1.0f;
			m_spPrevAnimation = nullptr;
		}
		else
		{
			isBlending = true;
		}
	}

	// ② 全てのノードの行列を計算
	for (auto& rAnimNode : m_spAnimation->m_nodes)
	{
		UINT idx = rAnimNode.m_nodeOffset;
		Math::Matrix matCurrent = rNodes[idx].m_localTransform;
		rAnimNode.Interpolate(matCurrent, m_time);

		if (isBlending)
		{
			Math::Matrix matPrev = rNodes[idx].m_localTransform;
			for (auto& prevNode : m_spPrevAnimation->m_nodes)
			{
				if (prevNode.m_nodeOffset == idx)
				{
					prevNode.Interpolate(matPrev, m_prevTime);
					break;
				}
			}

			// 行列を分解して補間（混ぜ合わせ）
			Math::Vector3 curS, prevS, curT, prevT;
			Math::Quaternion curR, prevR;
			matCurrent.Decompose(curS, curR, curT);
			matPrev.Decompose(prevS, prevR, prevT);

			Math::Vector3 blendS = DirectX::XMVectorLerp(prevS, curS, blendRatio);
			Math::Quaternion blendR = DirectX::XMQuaternionSlerp(prevR, curR, blendRatio);
			Math::Vector3 blendT = DirectX::XMVectorLerp(prevT, curT, blendRatio);

			rNodes[idx].m_localTransform = Math::Matrix::CreateScale(blendS) *
				Math::Matrix::CreateFromQuaternion(blendR) *
				Math::Matrix::CreateTranslation(blendT);
		}
		else
		{
			rNodes[idx].m_localTransform = matCurrent;
		}
	}

	// ③ 時間を進める
	m_time += speed;
	if (m_time >= m_spAnimation->m_maxLength)
	{
		if (m_isLoop) m_time = 0.0f;
		else m_time = m_spAnimation->m_maxLength;
	}

	if (isBlending)
	{
		m_prevTime += speed;
		if (m_prevTime >= m_spPrevAnimation->m_maxLength)
		{
			m_prevTime = m_spPrevAnimation->m_maxLength;
		}
	}
}
