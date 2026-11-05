#include "../inc_KdCommon.hlsli"
#include "inc_KdPostProcessShader.hlsli"

Texture2D g_inputTex	: register(t0);
Texture2D g_blurTex		: register(t1);
Texture2D g_strongBlurTex : register(t2);
Texture2D g_depthTex	: register(t3);

SamplerState g_ss : register(s0);

cbuffer cb : register(b0)
{
	float g_nearClipDist;
	float g_farClipDist;
	
	float g_focusDistance;
	float g_focusForeRange;
	float g_focusBackRange;

	// 画面の色（HP が少ない時など。0 で何もしない）
	float g_grayscale;		// 白黒にする強さ（0～1）
	float g_darken;			// 全体を暗くする強さ（0～1）
	float g_vignette;		// 周辺減光の強さ（0～1。1 でふちが真っ黒）
	float g_vignetteStart;	// 周辺減光が始まる位置（画面の真ん中からの距離。0：真ん中、1：上下左右のふち）
};

// 被写界深度シェーダー
float4 main(VSOutput In) : SV_Target0
{
	float3 color = 0;
	
	float4 depthPixel = g_depthTex.Sample( g_ss, In.UV );
	float2 projXY = (In.UV * 2 - 1) * float2(1, -1);
	depthPixel =  mul(float4(projXY, depthPixel.r, 1), g_mProjInv );
	depthPixel.xyz /= depthPixel.w;
	float depth = saturate((depthPixel.z - g_nearClipDist) / (g_farClipDist - g_nearClipDist));
	
	float focusGap = g_focusDistance - depth;
	float focusRange = focusGap > 0 ? g_focusForeRange : g_focusBackRange;
	
	focusGap = abs(focusGap);
	
	float defaultPow = max( 1.0f - pow( focusGap / focusRange, 2 ), 0.0f );
	float strongBlurPow = saturate( focusGap / focusRange - 1.0f );
	float blurPow = saturate( 1.0f - defaultPow - strongBlurPow );
	
	color += g_inputTex.Sample( g_ss, In.UV ).rgb * defaultPow;
	color += g_blurTex.Sample(g_ss, In.UV).rgb * blurPow;
	color += g_strongBlurTex.Sample(g_ss, In.UV).rgb * strongBlurPow;

	//------------------------------------------
	// 画面の色（HP が少ない時に、白黒・暗く・ふちを暗くする）
	//------------------------------------------
	float lum = dot(color, float3(0.299, 0.587, 0.114));
	color = lerp(color, lum * float3(0.94, 0.97, 1.03), g_grayscale);	// 少し冷たい灰色にする
	color *= 1.0 - g_darken;
	float dist = length(In.UV * 2 - 1);									// 真ん中 0、上下左右のふち 1、角 約1.4
	float vig = pow(saturate((dist - g_vignetteStart) / 0.85), 1.3);
	color *= 1.0 - vig * g_vignette;

	return float4(color, 1);
}
