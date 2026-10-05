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
	~BaseChara() override {}

	void PostUpdate() override;
	void Update() override;
	void DrawLit() override;

	void GroundHit();
	void BumpHit();

	// アニメーション関連
	void SetAnimationData(const std::string& name, const std::shared_ptr<KdAnimationData>& animData);
	virtual void ChangeAnimation(const std::string& animName, bool isLoop = true, bool forceRestart = false, float blendFrame = 0.0f);
	void LoadAnimations(const std::vector<AnimLoadInfo>& loadList);
	void SetAnimationSpeed(float speed) { m_animSpeed = speed; }
	float GetAnimTime() const { return m_animator.GetTime(); }
	bool IsAnimEnd() const { return m_animator.IsAnimationEnd(); }

	// ルートモーション関連
	void SetUseRootMotion(bool use) { m_useRootMotion = use; }
	void SetRootMotionScale(float scale) { m_rootScale = scale; }

	Math::Vector3 GetPos() const override { return m_pos; }
	void SetPos(const Math::Vector3& pos) override { m_pos = pos; }

	virtual void OnDamage(int damage, bool isCritical = false) {}

protected:

	void Release();

	// 無敵時間を1フレーム分進める
	void UpdateInvincibleTimer();

	// ルートモーションの移動量をY軸回転(ラジアン)で向きを合わせて座標に加算する
	void ApplyRootMotion(float scale, float rotYRad);

	Math::Vector3					m_pos;
	std::shared_ptr<KdModelWork>	m_model;
	float							m_gravity = 0.0f;

	// 1フレームあたりのルートモーション移動量
	Math::Vector3					m_rootMoveDelta = Math::Vector3::Zero;
	Math::Vector3					m_prevAnimRootPos = Math::Vector3::Zero;
	bool							m_useRootMotion = false;	// ルートモーションを使うかどうか
	float							m_rootScale = 1.0f;			// ルートモーションの移動倍率

	std::unordered_map<std::string, std::shared_ptr<KdAnimationData>> m_animMap;
	std::string						m_currentAnimName = "";
	KdAnimator						m_animator;
	float							m_animSpeed = 1.0f;

	// ステータス
	int								m_hp = 100;					// 現在のHP
	int								m_maxHp = 100;				// 最大HP
	int								m_invincibleTimer = 0;		// 無敵時間（フレーム）
};
