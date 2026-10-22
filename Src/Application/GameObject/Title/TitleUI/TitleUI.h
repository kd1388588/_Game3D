#pragma once
#include "../../../Utility/InputHelper.h"
#include "../../UI/Text/UIText.h"

// タイトル画面の2D表示（ロゴ・メニュー・操作説明・画面の演出）
// ロゴやメニューは時間に合わせて順番に現れ、その後も光り続ける
class TitleUI : public KdGameObject
{
public:

	// メニューで決定された内容
	enum class Result
	{
		None,		// まだ決まっていない
		Start,		// ゲーム開始
		Exit,		// ゲーム終了
	};

	TitleUI() {}
	~TitleUI() override {}

	void Init()			override;
	void Update()		override;
	void DrawSprite()	override;

	// 決定してフェードアウトが終わったら、決定した内容を返す
	Result GetResult() const { return m_result; }

private:

	// メニューの項目
	enum class MenuItem
	{
		Start,
		Controls,
		Exit,
		Num,
	};

	// 画面の状態
	enum class State
	{
		Intro,		// ロゴ・メニューが現れている途中
		Select,		// メニュー選択中
		Controls,	// 操作説明を表示中
		Decided,	// 決定した（決定した項目を点滅させてからフェードアウト）
	};

	// このフレームの入力
	struct Input
	{
		bool up = false;
		bool down = false;
		bool decide = false;	// Enter・Space
		bool click = false;		// 左クリック
		bool back = false;		// BackSpace・右クリック
	};

	// 全てのキーの入力を更新する（押した瞬間を正しく判定するため、毎フレーム全てのキーを更新する）
	void UpdateInput();

	// マウスの位置を、UIの座標（画面の中央が原点、上が＋Y）で取得する（取れなければ false）
	bool GetMouseUIPos(Math::Vector2& outPos) const;

	// マウスが乗っているメニューの項目（乗っていなければ -1）
	int GetHoveredMenuIndex(const Math::Vector2& mousePos) const;

	// 状態を切り替える
	void ChangeState(State state);

	// 状態ごとの処理
	void UpdateIntro();
	void UpdateSelect();
	void UpdateControls();
	void UpdateDecided();

	// カーソル・項目の大きさなど、なめらかに変化させる値の更新
	void UpdateMenuAnimation();

	// メニューの項目を決定した
	void Decide(MenuItem item);

	// メニューの項目の中心の位置
	static Math::Vector2 GetMenuItemPos(int index);

	// 描画（奥から順に呼ぶ）
	void DrawVignette() const;
	void DrawGuide() const;
	void DrawLogo() const;
	void DrawMenu() const;
	void DrawControls() const;
	void DrawFade() const;

	// 光の玉のテクスチャを、中心・大きさ・色を指定して描く
	void DrawGlow(const Math::Vector2& center, const Math::Vector2& size, const Math::Color& color) const;

	// 演出の進み具合（startFrame で0、startFrame + duration で1）
	float CalcProgress(int startFrame, int duration) const;

	// 操作説明の1行（左揃えで表示するため、左端の座標から中心を求めてセットする）
	void AddControlsRow(const std::string& key, const std::string& action, float y);

	static std::shared_ptr<UIText> CreateText(const std::string& text, UIText::FontSize fontSize);

	// 文字列
	std::shared_ptr<UIText>					m_logo = nullptr;
	std::shared_ptr<UIText>					m_subTitle = nullptr;
	std::shared_ptr<UIText>					m_selectGuide = nullptr;	// メニュー選択中の操作案内
	std::shared_ptr<UIText>					m_controlsGuide = nullptr;	// 操作説明を表示中の操作案内
	std::vector<std::shared_ptr<UIText>>	m_menuTexts;
	std::shared_ptr<UIText>					m_controlsTitle = nullptr;
	std::vector<std::shared_ptr<UIText>>	m_controlsTexts;

	// テクスチャ
	std::shared_ptr<KdTexture>				m_glowTex = nullptr;
	std::shared_ptr<KdTexture>				m_vignetteTex = nullptr;

	Math::Vector2							m_screenSize = { 1280.0f, 720.0f };

	State									m_state = State::Intro;
	Result									m_result = Result::None;
	MenuItem								m_decidedItem = MenuItem::Start;
	Input									m_input;
	Math::Vector2							m_prevMousePos = Math::Vector2::Zero;	// 前のフレームのマウスの位置
	int										m_frame = 0;			// タイトル画面が始まってからのフレーム数
	int										m_stateFrame = 0;		// 今の状態になってからのフレーム数
	int										m_selectIndex = 0;		// 選択中のメニュー
	float									m_cursorY = 0.0f;		// カーソルの高さ（選択中の項目へなめらかに移動する）
	float									m_menuMaxHalfWidth = 0.0f;	// 一番長いメニューの項目の幅の半分
	std::vector<float>						m_menuScales;			// 項目ごとの拡大率（選択中の項目を少し大きくする）
	float									m_controlsOpen = 0.0f;	// 操作説明の開き具合（0～1）

	// 入力（押した瞬間の判定用）
	InputHelper::KeyTrigger					m_upKey{ 'W' };
	InputHelper::KeyTrigger					m_upArrowKey{ VK_UP };
	InputHelper::KeyTrigger					m_downKey{ 'S' };
	InputHelper::KeyTrigger					m_downArrowKey{ VK_DOWN };
	InputHelper::KeyTrigger					m_enterKey{ VK_RETURN };
	InputHelper::KeyTrigger					m_spaceKey{ VK_SPACE };
	InputHelper::KeyTrigger					m_clickKey{ VK_LBUTTON };
	InputHelper::KeyTrigger					m_backKey{ VK_BACK };
	InputHelper::KeyTrigger					m_rightClickKey{ VK_RBUTTON };
};
