#pragma once
#include "../Base/BaseGage.h"

class UIText;

// HP の表示（FF7R 風。画面の右下のステータスパネル）
// ・パネルの背景（斜めの板）もここで描く（ATB・モードの表示は、この上に ATBGage が描く）
// ・大きな HP の数字と、斜めの HP ゲージ。減った分は白く残ってから、遅れて縮む
// ・HP が少ない時は、ゲージと数字を赤～オレンジでゆっくり点滅させる（ステージエディタの「画面」タブ）
class HPGage : public BaseGage
{
public:
	HPGage() {}
	~HPGage() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	void SetHpRatio(float ratio) { m_hpRatio = ratio; }
	void SetHp(int hp, int maxHp);

	// ステータスパネルの位置・大きさ（ATBGage も、この中に並べる）
	static constexpr float kPanelCenterX	= 371.0f;
	static constexpr float kPanelCenterY	= -293.0f;
	static constexpr float kPanelHalfW		= 235.0f;
	static constexpr float kPanelHalfH		= 33.0f;

private:

	void Release();

	float	m_hpRatio = 1.0f;
	float	m_lagRatio = 1.0f;		// 減った分を白く残しておく割合（少し遅れて m_hpRatio に近づく）
	int		m_hp = -1;
	int		m_maxHp = -1;
	int		m_frame = 0;			// 点滅に使う、経過フレーム数

	std::shared_ptr<UIText>	m_hpText = nullptr;		// 大きな数字
	std::shared_ptr<UIText>	m_maxText = nullptr;	// 「/ 100」
};
