#pragma once
#include "../Base/BaseGage.h"

class UIText;

// ATBゲージ（FF7リメイク風）
// ・ゲージは2本。攻撃を当てたり時間がたつとたまり、1本たまるごとにアビリティを1回出せる
// ・ゲージの上に、今のバトルモード（OPERATOR / PUNISHER）を表示する
class ATBGage : public BaseGage
{
public:
	ATBGage() {}
	~ATBGage() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	// たまっている量（ゲージ何本分か。例：1.5 なら1本目が満タン、2本目が半分）
	void SetAtbValue(float value) { m_atbValue = value; }

	// モードの表示を変える（変わった時は文字を少し大きくして目立たせる）
	void SetPunisherMode(bool isPunisher);

private:

	// ゲージ1本を描く（left：左端のX座標、ratio：たまっている割合 0～1）
	void DrawBar(int index, int left, float ratio) const;

	float					m_atbValue = 0.0f;
	bool					m_isPunisher = false;
	bool					m_isModeSet = false;	// 一度でもモードを設定したか（最初の設定では目立たせない）
	int						m_frame = 0;			// 経過フレーム（満タンのゲージを光らせる用）
	int						m_modeFlashTimer = 0;	// モードが変わった時の演出の残りフレーム
	std::vector<int>		m_barFlashTimers;		// ゲージがたまった瞬間に白く光らせる残りフレーム（1本ごと）
	std::vector<bool>		m_isBarFull;			// 前のフレームに満タンだったか（1本ごと）

	std::shared_ptr<UIText>	m_operatorText = nullptr;
	std::shared_ptr<UIText>	m_punisherText = nullptr;
};
