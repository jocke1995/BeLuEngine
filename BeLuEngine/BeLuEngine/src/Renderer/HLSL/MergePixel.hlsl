#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

float4 GammaCorrectedColor(float4 inputColor)
{
	const float gamma = 2.2;
	return pow(inputColor, 1.0 / gamma);
}

float4 TonemapReinhard(float4 inputColor)
{
	return inputColor / (inputColor + float4(1.0f, 1.0f, 1.0f, 1.0f));
}

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	float4 sceneColor = textures[dhIndices.index1].Sample(BilinearClamp, input.uv);
	float4 reflData = textures[dhIndices.index2].Sample(BilinearClamp, input.uv);

	sceneColor += float4(reflData.rgb, 1.0f);

	// Combine
	float4 finalColor = sceneColor;

	// HDR tone mapping
	finalColor = TonemapReinhard(finalColor);

	// GammaCorrect
	//finalColor = GammaCorrectedColor(finalColor);

	//reinhard += float4(reflData.rgb, 1.0f);
	//reinhard = saturate(reinhard);
	return float4(finalColor.rgb, 1.0f);
}
