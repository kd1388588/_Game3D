#pragma once
#include "../../Character/Base/BaseChara.h"

// タイトル画面の背景で、待機と連続攻撃を繰り返して見せるキャラクター
class TitleChara : public BaseChara
{
public:

	TitleChara() {}
	~TitleChara() override {}

	void Init()			override;
	void Update()		override;
	void DrawLit()		override;

	// カメラが注目する位置（胸のあたり）
	Math::Vector3 GetLookAtPos() const;

private:

	// 演技の1区切り
	struct ShowcaseStep
	{
		std::string	animName;			// 再生するアニメーション
		int			attackType = 0;		// 攻撃の種類（0なら攻撃ではない）
		int			attackStep = 0;		// 攻撃の段数
		int			holdFrame = 0;		// ループアニメーションを続けるフレーム数（0ならアニメーションの最後まで）
	};

	// 次の演技に進む
	void StartStep(size_t index);

	// 演技を進める（時間・アニメーションの終わりで次の演技へ）
	void UpdateShowcase();

	// 攻撃の振り始めに斬撃エフェクトを再生する
	void PlaySlashEffect(const ShowcaseStep& step);

	void UpdateWorldMatrix();

	// 読み込むアニメーションのリストを作成
	static std::vector<AnimLoadInfo> CreateAnimList();

	// 演技の順番を作成（待機→連続攻撃を、攻撃の種類を変えながら繰り返す）
	static std::vector<ShowcaseStep> CreateShowcase();

	// 攻撃の種類・段数に対応する斬撃エフェクトの名前
	static const std::string& GetSlashEffectName(int attackType, int attackStep);

	std::vector<ShowcaseStep>	m_showcase;
	size_t						m_stepIndex = 0;
	int							m_stepTimer = 0;			// 今の演技を始めてからのフレーム数
	bool						m_isEffectPlayed = false;	// 今の攻撃で斬撃エフェクトを出したか
	float						m_angle = 180.0f;			// 向き（度）。180度でカメラ（-Z側）の方を向く
};
