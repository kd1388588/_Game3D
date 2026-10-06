#pragma once

// アニメーションキー(クォータニオン
struct KdAnimKeyQuaternion
{
	float				m_time = 0;		// 時間
	Math::Quaternion	m_quat;			// クォータニオンデータ
};

// アニメーションキー(ベクトル
struct KdAnimKeyVector3
{
	float				m_time = 0;		// 時間
	Math::Vector3		m_vec;			// 3Dベクトルデータ
};

//============================
// アニメーションデータ
//============================
struct KdAnimationData
{
	// アニメーション名
	std::string		m_name;
	// アニメの長さ
	float			m_maxLength = 0;

	// １ノードのアニメーションデータ
	struct Node
	{
		int			m_nodeOffset = -1;	// 対象モデルノードのOffset値

		// 各チャンネル
		std::vector<KdAnimKeyVector3>		m_translations;	// 位置キーリスト
		std::vector<KdAnimKeyQuaternion>	m_rotations;	// 回転キーリスト
		std::vector<KdAnimKeyVector3>		m_scales;		// 拡縮キーリスト

		void Interpolate(Math::Matrix& rDst, float time);
		bool InterpolateTranslations(Math::Vector3& result, float time);
		bool InterpolateRotations(Math::Quaternion& result, float time);
		bool InterpolateScales(Math::Vector3& result, float time);
	};

	// 全ノード用アニメーションデータ
	std::vector<Node>	m_nodes;
};

class KdAnimator
{
public:

	//inline void SetAnimation(const std::shared_ptr<KdAnimationData>& rData, bool isLoop = true)
	//{
	//	m_spAnimation = rData;
	//	m_isLoop = isLoop;

	//	m_time = 0.0f;
	//}

	//////////////////////////////////////////////////////////////////////////////////////////
	// 0915 攻撃アニメーションの制御のため追加 戻す際は上のプログラムのコメントを外す
	//////////////////////////////////////////////////////////////////////////////////////////

	inline void SetAnimation(const std::shared_ptr<KdAnimationData>& rData, bool isLoop = true, float blendFrame = 0.0f)
	{
		// ブレンドフレームが指定されていて、かつ既にアニメーションが再生中の場合
		if (m_spAnimation && blendFrame > 0.0f)
		{
			// 今のアニメーションを「過去のアニメ」として退避させる
			m_spPrevAnimation = m_spAnimation;
			m_prevTime = m_time;
			m_blendDuration = blendFrame;
			m_currentBlendTimer = 0.0f;
		}
		else
		{
			// ブレンドしない場合は過去のアニメをリセット
			m_spPrevAnimation = nullptr;
		}

		m_spAnimation = rData;
		m_isLoop = isLoop;
		m_time = 0.0f;
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	// アニメーションが終了してる？
	bool IsAnimationEnd() const
	{
		if (m_spAnimation == nullptr) { return true; }
		if (m_time >= m_spAnimation->m_maxLength) { return true; }

		return false;
	}

	// アニメーションの更新
	void AdvanceTime(std::vector<KdModelWork::Node>& rNodes, float speed = 1.0f);

	float GetTime() const { return m_time; }

	// 再生位置を指定する（途中のフレームから再生したい場合に使用）
	void SetTime(float time)
	{
		m_time = time;
		if (m_spAnimation && m_time > m_spAnimation->m_maxLength)
		{
			m_time = m_spAnimation->m_maxLength;
		}
	}

private:

	std::shared_ptr<KdAnimationData>	m_spAnimation = nullptr;	// 再生するアニメーションデータ
	float m_time = 0.0f;
	bool m_isLoop = false;

	std::shared_ptr<KdAnimationData>	m_spPrevAnimation = nullptr; // 前のアニメーション
	float m_prevTime = 0.0f;           // 前のアニメーションの再生時間
	float m_blendDuration = 0.0f;      // ブレンドにかける総フレーム数
	float m_currentBlendTimer = 0.0f;  // 現在のブレンド進行度
};
