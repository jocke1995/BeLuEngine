#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

float4 TonemapReinhard(float4 inputColor)
{
	return inputColor / (inputColor + float4(1.0f, 1.0f, 1.0f, 1.0f));
}

static const float3x3 ACESInputMat =
{
	{0.59719, 0.35458, 0.04823},
	{0.07600, 0.90834, 0.01566},
	{0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
static const float3x3 ACESOutputMat =
{
	{ 1.60475, -0.53108, -0.07367},
	{-0.10208,  1.10813, -0.00605},
	{-0.00327, -0.07276,  1.07602}
};

float3 RRTAndODTFit(float3 v)
{
	float3 a = v * (v + 0.0245786f) - 0.000090537f;
	float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
	return a / b;
}

float4 TonemapACES(float4 inputColor)
{
	float3 color = inputColor.rgb;

	color = mul(ACESInputMat, color);

	// Apply RRT and ODT
	color = RRTAndODTFit(color);

	color = mul(ACESOutputMat, color);

	return float4(color.rgb, 1.0f);
}

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	float4 sceneColor = textures[dhIndices.index1].Sample(BilinearClamp, input.uv);
	//float4 reflData = textures[dhIndices.index2].Sample(BilinearClamp, input.uv);
	//
	//sceneColor += float4(reflData.rgb, 1.0f);

	// Combine
	float4 finalColor = sceneColor;

	// HDR tone mapping
	//finalColor = TonemapReinhard(finalColor);
	finalColor = TonemapACES(finalColor);

	//reinhard += float4(reflData.rgb, 1.0f);
	//reinhard = saturate(reinhard);
	return float4(finalColor.rgb, 1.0f);
}
