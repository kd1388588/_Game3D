#include "inc_KdStandardShader.hlsli"
#include "../inc_KdCommon.hlsli"

//================================
// 頂点シェーダ：影深度
//================================
VSOutputGenShadow main(
	float4 pos : POSITION,		// 頂点座標
	float2 uv : TEXCOORD0,		// テクスチャUV座標
	float4 color : COLOR,		// 頂点カラー
	float3 normal : NORMAL,		// 法線
	float3 tangent : TANGENT,	// 接線
	uint4 skinIndex : SKININDEX,	// スキンメッシュのボーンインデックス（VS_Lit と同じ）
	float4 skinWeight : SKINWEIGHT	// ボーンの影響度（VS_Lit と同じ）
)
{
	// スキニング（VS_Lit と同じ処理。これが無いと、影がアニメーションせずに最初の姿勢のまま残る）---------------->
	if (g_IsSkinMeshObj)
	{
		// 行列を合成
		row_major float4x4 mBones = 0; // 行列を0埋め
		[unroll]
		for (int i = 0; i < 4; i++)
		{
			mBones += g_mBones[skinIndex[i]] * skinWeight[i];
		}

		// 座標に適用
		pos = mul(pos, mBones);
	}
	// <----------------スキニング

	VSOutputGenShadow Out;
	
	// キャラクターの座標変換 : ローカル座標系 -> ワールド座標系へ変換
	Out.Pos = mul(pos, g_mWorld);
	
	// カメラの逆向きに変換 : ワールド座標系 -> ビュー座標系 -> 射影座標系へ変換
	Out.Pos = mul(Out.Pos, g_DL_mLightVP);

	// 射影行列を変換されないように保存
	Out.pPos = Out.Pos;
	
	// UV座標
	Out.UV = uv;

	Out.Color = color;

	// 出力
	return Out;
}
