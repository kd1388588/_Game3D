#pragma once

// ============================================================
// ゲームの時間の速さ（スローモーション用）
// ・1.0 で普通の速さ、0.1 なら 10分の1 の速さ（コマンドメニューを開いている間など）
// ・キャラのアニメーション・重力・エフェクトがこの速さで動く
//
// 【構成】SetScale で速さを変える（ATB コマンドを開く・閉じる時。PlayerBattle.cpp）
//         ・BaseChara::Update：アニメーションと重力に GetScale() をかける
//         ・Enemy：IsSlow() の間は「考える処理」を止め、飛びかかりの移動に GetScale() をかける
//         ・Effekseer：SetScale の中で、エフェクトの再生速度も同じにする
// 【なぜゲーム全体で 1 つか】スローは「世界全体の時間」なので、キャラごとに持つと
//         止め忘れ・戻し忘れが起きる。1 か所で変えれば、全部が同じ速さになる
// 【なぜ static だけのクラスか】作る必要が無く、どこからでも GameTime::GetScale() で読めるようにするため
//         （inline static の変数は、どの .cpp から使っても 1 つだけになる。C++17）
// 【参考】Game Programming Patterns「Game Loop」（ゲームの時間と、実際の時間を分けて扱う考え方）
//         https://gameprogrammingpatterns.com/game-loop.html
// ============================================================
class GameTime
{
public:

	// 時間の速さを変える
	static void SetScale(float scale)
	{
		s_scale = std::clamp(scale, 0.0f, 1.0f);
		KdEffekseerManager::GetInstance().SetUpdateSpeed(s_scale);
	}

	static float GetScale() { return s_scale; }

	// スローモーション中か（キャラの考える処理（攻撃するか・どこへ進むか）を止めるのに使う）
	static bool IsSlow() { return s_scale < 1.0f; }

	// 普通の速さに戻す（シーンの切り替え時など）
	static void Reset() { SetScale(1.0f); }

private:

	inline static float s_scale = 1.0f;
};
