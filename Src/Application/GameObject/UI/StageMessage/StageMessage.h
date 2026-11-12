#pragma once

class UIText;

// ============================================================
// 【役割】ステージの進み具合を知らせる表示
//         ・画面の真ん中より少し上に、短いメッセージを出して、ふわっと消す（「鍵を手に入れた」など）
//         ・鍵を持っている間は、HUD（左下）の上に「鍵」と出しておく
// 【使い方】GameScene が Show("…") を呼ぶ。鍵を持ったら SetHasKey(true)
// 【なぜ UIText を中に持つのか】文字の作り直し（フォントの画像を作る）は重いので、
//         文字が変わった時だけ作り直し、毎フレームは位置と色だけを変える
// ============================================================
class StageMessage : public KdGameObject
{
public:
	StageMessage() {}
	~StageMessage() override {}

	void Init() override;
	void Update() override;
	void DrawSprite() override;

	// メッセージを出す（frames：出しておくフレーム数。最後の方でふわっと消える）
	void Show(const std::string& text, int frames = 150);

	// 鍵を持っているか（持っている間は、左下に「鍵」と出す）
	void SetHasKey(bool hasKey) { m_hasKey = hasKey; }

private:

	std::shared_ptr<UIText>	m_text = nullptr;
	std::shared_ptr<UIText>	m_keyText = nullptr;
	int						m_timer = 0;
	int						m_maxTimer = 0;
	bool					m_hasKey = false;
};
