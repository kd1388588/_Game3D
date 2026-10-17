#pragma once
#include "../Base/BaseGage.h"

class UIText;

// バトル用のゲージ表示（FF7リメイク風。HPゲージのまわりに並べる）
// ・ATBゲージ（2本）：攻撃を当てたり時間がたつとたまる。アビリティで使う
// ・アビリティ（攻撃04・05）：使えるか・CT（クールタイム）の残り・使うゲージの本数
// ・WPゲージ：ためると G キーで覚醒できる（覚醒中は残り時間）
// ・今のバトルモード（OPERATOR / PUNISHER）と、モードの固定の残り時間・ガード中の表示
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

	// モードを切り替えられない残りの割合（0 なら切り替えられる）
	void SetModeLockRatio(float ratio) { m_modeLockRatio = ratio; }

	// アビリティの状態（index：0 が攻撃04、1 が攻撃05）
	// canUse：今使えるか、ctRatio：CTの残りの割合（0～1）、cost：使うATBゲージの本数
	void SetAbilityState(int index, bool canUse, float ctRatio, int cost);

	// WPゲージ（ratio：たまっている割合。覚醒中は残り時間の割合）
	void SetWpState(float ratio, bool isAwakening) { m_wpRatio = ratio; m_isAwakening = isAwakening; }

	// ガード中か
	void SetGuarding(bool isGuarding) { m_isGuarding = isGuarding; }

private:

	// ATBゲージ1本を描く（left：左端のX座標、ratio：たまっている割合 0～1）
	void DrawBar(int index, int left, float ratio) const;

	// アビリティの枠を1つ描く
	void DrawAbilitySlot(int index) const;

	// WPゲージを描く
	void DrawWpBar() const;

	// 明るさを周期的に変えた色（満タンで光らせる用）
	Math::Color GetGlowColor(const Math::Color& baseColor) const;

	// アビリティ1つ分の状態
	struct AbilitySlotState
	{
		bool	canUse = false;
		float	ctRatio = 0.0f;
		int		cost = 1;
		int		readyFlashTimer = 0;	// 使えるようになった瞬間に光らせる残りフレーム
	};

	float					m_atbValue = 0.0f;
	bool					m_isPunisher = false;
	bool					m_isModeSet = false;	// 一度でもモードを設定したか（最初の設定では目立たせない）
	float					m_modeLockRatio = 0.0f;
	float					m_wpRatio = 0.0f;
	bool					m_isAwakening = false;
	bool					m_isGuarding = false;
	int						m_frame = 0;			// 経過フレーム（満タンのゲージを光らせる用）
	int						m_modeFlashTimer = 0;	// モードが変わった時の演出の残りフレーム
	std::vector<int>		m_barFlashTimers;		// ゲージがたまった瞬間に白く光らせる残りフレーム（1本ごと）
	std::vector<bool>		m_isBarFull;			// 前のフレームに満タンだったか（1本ごと）
	std::vector<AbilitySlotState>			m_abilities;

	std::shared_ptr<UIText>					m_operatorText = nullptr;
	std::shared_ptr<UIText>					m_punisherText = nullptr;
	std::shared_ptr<UIText>					m_guardText = nullptr;
	std::shared_ptr<UIText>					m_wpText = nullptr;
	std::shared_ptr<UIText>					m_wpReadyText = nullptr;	// WPが満タンの時の「G：覚醒」
	std::vector<std::shared_ptr<UIText>>	m_slotKeyTexts;				// アビリティの枠の「04」「05」
};
