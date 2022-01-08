#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;

float4 GammaCorrect(float4 finalColor)
{
	const float gamma = 2.2f;
	return float4(pow(finalColor.rgb, float3(1 / gamma, 1 / gamma, 1 / gamma)), 1.0f);
}

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

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int finalColorReadIndex = rootConstantUints.index0;
	unsigned int finalColorWriteIndex = rootConstantUints.index1;
	float4 finalColor = textures[finalColorReadIndex][dispatchThreadID.xy];

	// Tonemap
	finalColor = TonemapACES(finalColor);
	//finalColor = TonemapReinhard(finalColor);

	finalColor = GammaCorrect(finalColor);
	texturesUAV[finalColorWriteIndex][dispatchThreadID.xy] = saturate(finalColor);
}