#pragma once

class Player;

// ============================================================
// 【役割】鍵の部屋の敵を全員倒すと出てくる鍵（拾うとボスの部屋の扉が開けられる）
// 【構成】・Init      ：鍵の画像（Key.png）の板を作る
//         ・Update    ：ふわふわ上下に動かし、金色の光（点光源）を出す。プレイヤーが近づいたら拾われる
//         ・DrawEffect：カメラの方を向けて、加算合成で描く（光る物なので）
// 【使い方】GameScene が、鍵の部屋の敵を全員倒した時に作る（SetPos・SetTarget の後に Init）
//          毎フレーム IsPicked() を見て、拾われたら鍵を持っていることにする
// 【なぜモデルではなく板か】鍵のモデルが無いため。光る小物は、炎（StageLights）と同じく
//          カメラを向いた板（ビルボード）にすると、どの向きから見ても同じ形に見える
// 画像は Tools/MakeKeyTexture.py で作成
// ============================================================
class KeyItem : public KdGameObject
{
public:
	KeyItem() {}
	~KeyItem() override {}

	void Init() override;
	void Update() override;
	void DrawEffect() override;

	// 置く場所（台の上。ここを中心に上下に動く）
	void SetPos(const Math::Vector3& pos) override { m_basePos = pos; }
	Math::Vector3 GetPos() const override { return m_basePos; }

	// 拾う人（プレイヤー）
	void SetTarget(const std::shared_ptr<Player>& player) { m_wpPlayer = player; }

	// 拾われたか（拾われた次のフレームに、シーンから消える）
	bool IsPicked() const { return m_isPicked; }

private:

	std::shared_ptr<KdSquarePolygon>	m_spPolygon = nullptr;
	std::weak_ptr<Player>				m_wpPlayer;
	Math::Vector3						m_basePos = Math::Vector3::Zero;
	int									m_frame = 0;
	float								m_appearRate = 0.0f;	// 出てくる時の大きさ（0 → 1）
	bool								m_isPicked = false;
};
