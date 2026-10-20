#pragma once
#include "../Base/BaseGage.h"

class UIText;

// バトル用のゲージ表示（FF7リメイク風。HPゲージのまわりに並べる）
// ・ATBゲージ（2本）：攻撃を当てたり時間がたつとたまる。アビリティで使う
// ・アビリティ（攻撃04・05）：使えるか・CT（クールタイム）の残り・使うゲージの本数
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


	// ガード中か
	void SetGuarding(bool isGuarding) { m_isGuarding = isGuarding; }

	// ガードの耐久値（ratio：残りの割合）と、崩されているか
	void SetGuardState(float ratio, bool isBroken) { m_guardRatio = ratio; m_isGuardBroken = isBroken; }

private:

	// 横に伸びるゲージを1本描く（背景・たまっている部分・枠。ATB・ガードのゲージで共通）
	// left：左端のX座標、centerY：中心の高さ、ratio：たまっている割合 0～1
	static void DrawGaugeBar(int left, int centerY, int width, int height, float ratio, const Math::Color& fillColor);

	// ATBゲージ1本を描く（left：左端のX座標、ratio：たまっている割合 0～1）
	void DrawBar(int index, int left, float ratio) const;

	// アビリティの枠を1つ描く
	void DrawAbilitySlot(int index) const;


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
	bool					m_isGuarding = false;
	float					m_guardRatio = 1.0f;
	bool					m_isGuardBroken = false;
	int						m_frame = 0;			// 経過フレーム（満タンのゲージを光らせる用）
	int						m_modeFlashTimer = 0;	// モードが変わった時の演出の残りフレーム
	std::vector<int>		m_barFlashTimers;		// ゲージがたまった瞬間に白く光らせる残りフレーム（1本ごと）
	std::vector<bool>		m_isBarFull;			// 前のフレームに満タンだったか（1本ごと）
	std::vector<AbilitySlotState>			m_abilities;

	std::shared_ptr<UIText>					m_operatorText = nullptr;
	std::shared_ptr<UIText>					m_punisherText = nullptr;
	std::shared_ptr<UIText>					m_guardText = nullptr;
	std::shared_ptr<UIText>					m_breakText = nullptr;	// ガードを崩された時の「BREAK」
	std::vector<std::shared_ptr<UIText>>	m_slotKeyTexts;				// アビリティの枠の「04」「05」

	// どのキーで使えるかの表示（四角い枠の中にキーの名前）
	struct KeyBadge
	{
		std::shared_ptr<UIText>	text;
		Math::Vector2			pos;
	};
	KeyBadge								m_modeKeyBadge;		// モードの文字の左の「E」
	KeyBadge								m_commandKeyBadge;	// アビリティの枠の右の「F」

	static KeyBadge CreateKeyBadge(const std::string& key, const Math::Vector2& pos);
	static void DrawKeyBadge(const KeyBadge& badge);
};
