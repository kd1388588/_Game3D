#pragma once

// 廃墟のステージの光・フォグ・影の設定（タイトルとゲームで同じ見た目にする。ゲームは環境光を明るく、霧を薄くする）
//
// 【役割】シーンを作った時に 1 回、光（平行光・環境光）・霧・影の範囲を設定する
// 【使う所】TitleScene::Init で Apply()、GameScene::Init で Apply(true)
//           GameScene::Event で毎フレーム SetShadowCenter（影の中心をプレイヤーにする）
// 【StageSettings との関係】ゲームの画面では、霧の値は毎フレーム StageSettings::ApplyFog で上書きされる
//           （ステージエディタで調整できるように）。ここの霧の値は、タイトルの画面で使われる
// 【なぜ平行光を斜め横からにするのか】
//   光が面に当たる角度で明るさが変わる（ランバート反射：明るさ ＝ 面の向き・光の向き）
//   真上や真後ろからだと、どの面も同じくらいの明るさになり、凹凸が見えない
// 【参考】ランバート反射・平行光・環境光の考え方
//   https://learn.microsoft.com/ja-jp/windows/win32/direct3d9/diffuse-lighting
namespace StageEnvironment
{
	// 平行光（月明かり）：斜め横から当てて、体や柱の凹凸に陰影を付ける（真後ろ・真上からだとのっぺりする）
	const Math::Vector3	kDirLightDir		= { -0.6f, -1.0f, 0.35f };
	const Math::Vector3	kDirLightColor		= { 1.8f, 1.75f, 1.65f };

	// 環境光：少し暗めの青にして、光の当たらない側との差を付ける
	const Math::Vector4	kAmbientLight		= { 0.22f, 0.24f, 0.32f, 1.0f };		// タイトル（暗いほうが雰囲気が出る）
	const Math::Vector4	kGameAmbientLight	= { 0.36f, 0.38f, 0.48f, 1.0f };		// ゲーム（影の中の敵や壁も見えるように明るく）

	// 距離フォグ：遠くを夜の空気の色に溶かして奥行きを出す（近くのキャラはほぼかからない）
	// 見え方：1 / exp(距離 × 濃さ) → 20m で約 0.79、50m で約 0.55 だけ元の色が残る
	const Math::Vector3	kDistanceFogColor	= { 0.10f, 0.11f, 0.16f };
	constexpr float		kDistanceFogDensity	= 0.012f;
	constexpr float		kGameDistanceFogDensity	= 0.007f;	// ゲームでは薄くする（部屋の奥の敵が霧に溶けないように）

	// 高さフォグ：地面の近くにうっすら霧（遠くほど濃い。近くのキャラの足元は隠さない）
	const Math::Vector3	kHeightFogColor		= { 0.12f, 0.13f, 0.18f };
	constexpr float		kHeightFogTop		= 1.0f;		// この高さより下に霧がかかる
	constexpr float		kHeightFogBottom	= -1.0f;	// この高さで一番濃い
	constexpr float		kHeightFogDistance	= 45.0f;	// カメラからこの距離で一番濃くなる

	// 影を作る範囲（幅・奥行きと、光の高さ）。プレイヤーを中心にする
	// 【なぜ 30m にしぼるのか】影は「光から見た深度の画像（シャドウマップ）」に描いて作る
	//   画像の大きさは決まっているので、範囲を広げるほど 1 ピクセルが大きくなり、影の形が粗くなる
	//   戦う所（プレイヤーのまわり）だけにしぼると、影がくっきりする
	// 【参考】シャドウマップ https://learn.microsoft.com/ja-jp/windows/win32/dxtecharts/common-techniques-to-improve-shadow-depth-maps
	const Math::Vector2	kShadowArea			= { 30.0f, 30.0f };
	constexpr float		kShadowLightHeight	= 40.0f;

	// シーンの初期化で呼ぶ（isGame：ゲームの画面なら true。戦いやすいように明るくする）
	inline void Apply(bool isGame = false)
	{
		KdAmbientController& ambient = KdShaderManager::Instance().WorkAmbientController();
		ambient.SetDirLight(kDirLightDir, kDirLightColor);
		ambient.SetAmbientLight(isGame ? kGameAmbientLight : kAmbientLight);
		ambient.SetFogEnable(true, true);
		ambient.SetDistanceFog(kDistanceFogColor, isGame ? kGameDistanceFogDensity : kDistanceFogDensity);
		ambient.SetheightFog(kHeightFogColor, kHeightFogTop, kHeightFogBottom, kHeightFogDistance);
		ambient.SetDirLightShadowArea(kShadowArea, kShadowLightHeight);
		ambient.ClearShadowCenter();
	}

	// 影を作る範囲の中心を決める（毎フレーム。プレイヤーの位置など）
	inline void SetShadowCenter(const Math::Vector3& center)
	{
		KdShaderManager::Instance().WorkAmbientController().SetShadowCenter(center);
	}
}
