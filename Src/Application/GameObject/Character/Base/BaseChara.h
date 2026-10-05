#pragma once
#include <unordered_map>
#include <memory>

struct AnimLoadInfo
{
	std::string name; 
	std::string path; 
};

class BaseChara : public KdGameObject
{
public:

	BaseChara() {}
	~BaseChara(){}

	void PostUpdate() override;
	void Update() override;
	void DrawLit() override;

	void GroundHit();
	void BumpHit();

	void SetAnimationData(const std::string& name, const std::shared_ptr<KdAnimationData>& animData);
	virtual void ChangeAnimation(const std::string& animName, bool isLoop = true, bool forceRestart = false, float blendFrame = 0.0f);
	void LoadAnimations(const std::vector<AnimLoadInfo>& loadList);
	Math::Vector3 GetPos() const override { return m_pos; }
	void SetPos(const Math::Vector3& pos) { m_pos = pos; }
	void SetAnimationSpeed(float speed) { m_animSpeed = speed; }

	virtual void OnDamage(int damage, bool isCritical = false) {}

protected:

	void Release();

	Math::Vector3					m_pos;
	std::shared_ptr<KdModelWork>	m_model;	
	float							m_gravity = 0.0f;

	// 1フレームあたりのルートモーション移動量
	Math::Vector3					m_rootMoveDelta = Math::Vector3::Zero;
	Math::Vector3					m_prevAnimRootPos = Math::Vector3::Zero;

	std::unordered_map<std::string, std::shared_ptr<KdAnimationData>> m_animMap;
	std::string						m_currentAnimName = "";
	KdAnimator						m_animator;
	float							m_animSpeed = 1.0f;

};