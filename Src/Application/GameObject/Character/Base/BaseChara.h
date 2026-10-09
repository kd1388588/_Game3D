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
	void SetAnimTime(float time) { m_animator.SetTime(time); }
	bool IsAnimEnd() const { return m_animator.IsAnimationEnd(); }
	bool HasAnimation(const std::string& animName) const;

	// 指定した足の骨がすべて接地している最初のフレームを求める（見つからなければ0）
	float CalcFootPlantFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const;

	// ルートモーション関連
	void SetUseRootMotion(bool use) { m_useRootMotion = use; }
	void SetRootMotionScale(float scale) { m_rootScale = scale; }

	Math::Vector3 GetPos() const override { return m_pos; }
	void SetPos(const Math::Vector3& pos) override { m_pos = pos; }

	virtual void OnDamage(int damage, bool isCritical = false) {}

	// 無敵時間中か（攻撃が当たってもダメージを受けない）
	bool IsInvincible() const { return m_invincibleTimer > 0; }

	// HPが残っているか
	bool IsAlive() const { return m_hp > 0; }

	// 現在の落下速度のまま落ち続けた場合、何フレーム後に着地するかを予測する
	// （maxFrames以内に着地しない・下に地面がない場合は -1）
	int PredictLandingFrames(int maxFrames) const;

protected:

	void Release();

	// 足元から下方向にレイを飛ばし、一番近い地面の座標を取得する
	bool FindGroundBelow(float range, Math::Vector3& outHitPos) const;

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

private:

	// 自分以外の全オブジェクトと当たり判定を行い、結果をまとめて返す
	// ※BaseChara.cpp内でのみ使用（定義も BaseChara.cpp）
	template<class ShapeInfo>
	std::list<KdCollider::CollisionResult> IntersectsOthers(const ShapeInfo& shape) const;

	// ルートボーンを探す
	static KdModelWork::Node* FindRootNode(std::vector<KdModelWork::Node>& nodes);

	// 別のファイルから読み込んだアニメーションを、このキャラのモデルに合わせて作り直す（リターゲット）
	// ・骨の対応は「番号」ではなく「名前」で行う（骨の並び順が違っても正しく動くように）
	// ・骨の長さはモデル側を使い、root / pelvis の移動量だけ体格の比率で拡大縮小する
	// 骨の並び・長さが元から一致している場合は、元のアニメーションをそのまま返す
	static std::shared_ptr<KdAnimationData> RetargetAnimation(const std::shared_ptr<KdAnimationData>& srcAnim,
		const KdModelData& srcModel, const KdModelData& dstModel, const std::string& debugName);
};
