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
	void SetAnimTime(float time) { m_animator.SetTime(time); m_prevMotionTime = -1.0f; }
	bool IsAnimEnd() const { return m_animator.IsAnimationEnd(); }
	bool HasAnimation(const std::string& animName) const;

	// 指定した足の骨がすべて接地している最初のフレームを求める（見つからなければ0）
	float CalcFootPlantFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const;

	// 指定した足の骨がすべて地面から離れる最初のフレームを求める（見つからなければ -1）
	// （ジャンプで、アニメーションの踏み切りに合わせて跳ぶために使う）
	float CalcFootLiftFrame(const std::string& animName, const std::vector<std::string>& footNodeNames) const;

	// ルートモーション関連
	void SetUseRootMotion(bool use) { m_useRootMotion = use; }
	void SetRootMotionScale(float scale) { m_rootScale = scale; }

	// アニメーションの「腰の水平移動」を、キャラの移動として取り出すかどうか
	// true にすると、腰が動いた分が m_rootMoveDelta に入り、
	// ルートモーションを使う時（SetUseRootMotion(true)）は腰がキャラの位置に固定される
	// （アニメーションが切り替わっても、体が元の位置に戻らなくなる）
	void SetExtractPelvisMotion(bool extract)
	{
		m_extractPelvisMotion = extract;
		m_prevMotionTime = -1.0f;	// 移動の基準を作り直す
		if (extract && !m_isMotionNodeReady) FindMotionNodes();
	}

	// 空中で、アニメーションで腰（体全体）が上がる分を止める
	// （プログラムでキャラを上に飛ばしているので、アニメーションでも上がると2重に高く見えるため）
	// true にした時の腰の高さ（立ち位置より低ければ立ち位置）より上には、腰が上がらなくなる
	void SetAirPelvisLock(bool isLock);

	// 落下中の重力の倍率（1.0 で普通。大きくすると、上がる時より落ちる時が速くなる）
	void SetFallGravityRate(float rate) { m_fallGravityRate = rate; }

	// アニメーションに「前へ進む動き」（腰の水平移動）が入っているか
	// （その場で足踏みするだけのアニメーションなら false）
	bool HasHorizontalMotion(const std::string& animName) const;

	// 再生中のアニメーションに「前へ進む動き」が入っているか
	// （腰の水平移動を取り出していない時は、常に false）
	bool CurrentAnimHasHorizontalMotion() const
	{
		return m_extractPelvisMotion && HasHorizontalMotion(m_currentAnimName);
	}

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

	// 腰の水平移動を取り出して m_rootMoveDelta に入れ、腰をキャラの位置に固定する
	// （poseTime：今回のポーズを作った再生時間）
	void UpdatePelvisMotion(float poseTime);

	// 指定したアニメーションの、指定した時間の「腰の水平位置」（立ち位置からのずれ）
	// pOutPelvisPos を渡すと、腰の位置（アニメーションの値そのまま）も受け取れる
	Math::Vector3 SampleMotionPos(const KdAnimationData& anim, float time, Math::Vector3* pOutPelvisPos = nullptr) const;

	// 全ての骨をモデルの初期姿勢に戻す
	// （アニメーションにキーが無い骨が、前のアニメーションの姿勢のまま残らないようにする）
	void ResetPoseToRest();

	// root・pelvis のノード番号と、腰の立ち位置を調べる（最初の1回だけ）
	void FindMotionNodes();

	Math::Vector3					m_pos;
	std::shared_ptr<KdModelWork>	m_model;
	float							m_gravity = 0.0f;

	// 1フレームあたりのルートモーション移動量
	Math::Vector3					m_rootMoveDelta = Math::Vector3::Zero;
	Math::Vector3					m_prevAnimRootPos = Math::Vector3::Zero;
	bool							m_useRootMotion = false;	// ルートモーションを使うかどうか
	float							m_rootScale = 1.0f;			// ルートモーションの移動倍率

	// 腰の水平移動の取り出し用
	bool							m_extractPelvisMotion = false;	// 腰の水平移動を取り出すか
	bool							m_isMotionNodeReady = false;	// ノード番号を調べ終わったか
	int								m_rootNodeIndex = -1;			// root のノード番号
	int								m_pelvisNodeIndex = -1;			// pelvis のノード番号
	Math::Vector3					m_pelvisRestPos = Math::Vector3::Zero;	// 腰の立ち位置（モデルの初期姿勢）
	float							m_prevMotionTime = -1.0f;		// 前回の再生時間（-1 は切り替え直後）
	Math::Vector3					m_prevMotionPos = Math::Vector3::Zero;	// 前回の腰の水平位置
	float							m_pelvisMaxY = -1.0f;			// 空中で腰が上がれる高さ（-1 は制限なし）

	float							m_fallGravityRate = 1.0f;		// 落下中の重力の倍率

	std::unordered_map<std::string, std::shared_ptr<KdAnimationData>> m_animMap;
	std::string						m_currentAnimName = "";
	std::shared_ptr<KdAnimationData> m_currentAnim = nullptr;		// 再生中のアニメーション
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

	// アニメーションを1フレームずつ再生し、指定した骨の高さを記録する（足の接地・離地の計算用）
	bool SampleNodeHeights(const std::string& animName, const std::vector<std::string>& nodeNames,
		std::vector<float>& outTimes, std::vector<std::vector<float>>& outHeights) const;

	// 別のファイルから読み込んだアニメーションを、このキャラのモデルに合わせて作り直す（リターゲット）
	// ・骨の対応は「番号」ではなく「名前」で行う（骨の並び順が違っても正しく動くように）
	// ・骨の長さはモデル側を使い、root / pelvis の移動量だけ体格の比率で拡大縮小する
	// 骨の並び・長さが元から一致している場合は、元のアニメーションをそのまま返す
	static std::shared_ptr<KdAnimationData> RetargetAnimation(const std::shared_ptr<KdAnimationData>& srcAnim,
		const KdModelData& srcModel, const KdModelData& dstModel, const std::string& debugName);
};
