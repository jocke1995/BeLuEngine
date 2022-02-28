#include "LightCalculations.hlsl"	// This includes "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float2 uv       : UV;
};

struct PS_OUTPUT
{
	float4 sceneColor: SV_TARGET0;
};

PS_OUTPUT PS_main(VS_OUT input)
{
	// Sample from textures
	float2 uv = float2(input.uv.x, input.uv.y);
	float3 albedo = textures[cbPerScene.gBufferAlbedo].Sample(Anisotropic16_Wrap, uv);
	float roughness = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uv).r;
	float metallic = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uv).g;
	float depth= textures[cbPerScene.depth].Sample(Anisotropic16_Wrap, uv).r;

	float3 normal = textures[cbPerScene.gBufferNormal].Sample(Anisotropic16_Wrap, uv);
	float3 emissive = textures[cbPerScene.gBufferEmissive].Sample(Anisotropic16_Wrap, uv);
	float3 reflection = textures[cbPerScene.reflectionTextureSRV].Sample(Anisotropic16_Wrap, uv).rgb;

	
	PS_OUTPUT output = (PS_OUTPUT)0;
	output.sceneColor = float4(1.0f, 0.0f, 1.0f, 1.0f);

	return output;
}
