#pragma once

// 廃墟のステージの光・フォグ・影の設定（タイトルとゲームで同じ見た目にする）
namespace StageEnvironment
{
	// 平行光（月明かり）：斜め横から当てて、体や柱の凹凸に陰影を付ける（真後ろ・真上からだとのっぺりする）
	const Math::Vector3	kDirLightDir		= { -0.6f, -1.0f, 0.35f };
	const Math::Vector3	kDirLightColor		= { 1.8f, 1.75f, 1.65f };

	// 環境光：少し暗めの青にして、光の当たらない側との差を付ける
	const Math::Vector4	kAmbientLight		= { 0.22f, 0.24f, 0.32f, 1.0f };

	// 距離フォグ：遠くを夜の空気の色に溶かして奥行きを出す（近くのキャラはほぼかからない）
	// 見え方：1 / exp(距離 × 濃さ) → 20m で約 0.79、50m で約 0.55 だけ元の色が残る
	const Math::Vector3	kDistanceFogColor	= { 0.10f, 0.11f, 0.16f };
	constexpr float		kDistanceFogDensity	= 0.012f;

	// 高さフォグ：地面の近くにうっすら霧（遠くほど濃い。近くのキャラの足元は隠さない）
	const Math::Vector3	kHeightFogColor		= { 0.12f, 0.13f, 0.18f };
	constexpr float		kHeightFogTop		= 1.0f;		// この高さより下に霧がかかる
	constexpr float		kHeightFogBottom	= -1.0f;	// この高さで一番濃い
	constexpr float		kHeightFogDistance	= 45.0f;	// カメラからこの距離で一番濃くなる

	// 影を作る範囲（幅・奥行きと、光の高さ）。プレイヤーを中心にする
	const Math::Vector2	kShadowArea			= { 30.0f, 30.0f };
	constexpr float		kShadowLightHeight	= 40.0f;

	// シーンの初期化で呼ぶ
	inline void Apply()
	{
		KdAmbientController& ambient = KdShaderManager::Instance().WorkAmbientController();
		ambient.SetDirLight(kDirLightDir, kDirLightColor);
		ambient.SetAmbientLight(kAmbientLight);
		ambient.SetFogEnable(true, true);
		ambient.SetDistanceFog(kDistanceFogColor, kDistanceFogDensity);
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
