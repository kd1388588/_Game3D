#include"Effect.h"
#include"../../../Framework/Effekseer/KdEffekseerManager.h"

void Effect::Init()
{}

void Effect::Update()
{
	// 再生中のエフェクトが存在するかチェック
	if (m_effect)
	{
		// エフェクトの再生が完全に終了したら
		if (!m_effect->IsPlaying())
		{
			// KdGameObjectの機能。リストから自動で削除される（寿命）
			m_isExpired = true;
		}
	}
	else
	{
		// エフェクトがセットされていなければ即座に消滅
		m_isExpired = true;
	}
}

void Effect::DrawUnLit()
{}

void Effect::SetEffect(const std::string& effectName, const Math::Vector3& pos, const float size, const float speed, const bool isLoop)
{
	// マネージャーに再生を依頼し、返ってきた管理用オブジェクトを保存する
	m_effect = KdEffekseerManager::GetInstance().Play(effectName, pos, size, speed, isLoop).lock();
}