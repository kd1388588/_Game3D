#pragma once

class UIText;
class Player;

// ゲーム中、画面の右下に出す操作キーの一覧
// ATBコマンドメニューを開いている間は、メニューと重ならないように消す
class ControlsGuide : public KdGameObject
{
public:
	ControlsGuide() {}
	~ControlsGuide()override {}

	void Init()override;
	void Update()override;
	void DrawSprite()override;

	void SetPlayer(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

private:

	// 1行分（キーの名前と、その操作）
	struct Row
	{
		std::shared_ptr<UIText>	key;
		std::shared_ptr<UIText>	action;
	};
	std::vector<Row>		m_rows;

	std::weak_ptr<Player>	m_wpPlayer;
	float					m_alpha = 1.0f;		// 表示の濃さ（メニューを開くと薄くなって消える）
};
