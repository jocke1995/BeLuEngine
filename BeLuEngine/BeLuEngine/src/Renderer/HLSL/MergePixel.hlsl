#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	float4 sceneColor = textures[dhIndices.index1].Sample(MIN_MAG_MIP_LINEAR_Wrap, input.uv);
	float4 reflData = textures[dhIndices.index2].Sample(MIN_MAG_MIP_LINEAR_Wrap, input.uv);

	sceneColor += float4(reflData.rgb, 1.0f);

	// Combine
	float4 finalColor = sceneColor;

	// HDR tone mapping
	float4 reinhard = finalColor / (finalColor + float4(1.0f, 1.0f, 1.0f, 1.0f));

	//reinhard += float4(reflData.rgb, 1.0f);
	//reinhard = saturate(reinhard);
	return float4(reinhard.rgb, 1.0f);
}
