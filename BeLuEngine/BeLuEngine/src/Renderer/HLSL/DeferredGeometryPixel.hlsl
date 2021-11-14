#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
	float3x3 tbn	: TBN;
};

struct PS_OUTPUT
{
	float4 AlbedoColor		: SV_TARGET0;
	float4 NormalColor		: SV_TARGET1;
	float4 MatColor			: SV_TARGET2;
	float4 EmissiveColor	: SV_TARGET3;
};

PS_OUTPUT PS_main(VS_OUT input)
{
	PS_OUTPUT output = (PS_OUTPUT)0;

	// Sample from textures
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo = textures[material.textureAlbedo].Sample(Anisotropic16_Wrap, uvScaled);
	float roughness = material.hasRoughnessTexture ? textures[material.textureRoughness].Sample(Anisotropic16_Wrap, uvScaled).r : material.roughnessValue;
	float metallic = material.hasMetallicTexture ? textures[material.textureMetallic].Sample(Anisotropic16_Wrap, uvScaled).r : material.metallicValue;
	float4 emissive = material.hasEmissiveTexture ? textures[material.textureEmissive].Sample(Anisotropic16_Wrap, uvScaled) : material.emissiveValue;
	
	float3 normal = float3(0.0f, 0.0f, 0.0f);
	if (material.hasNormalTexture)
	{
		normal = textures[material.textureNormal].Sample(Anisotropic16_Wrap, uvScaled).xyz;
		normal = normalize((2.0f * normal) - 1.0f);
		normal = normalize(mul(normal, input.tbn));
	}
	else
	{
		normal = input.norm.xyz;
	}

	output.AlbedoColor	 = albedo;
	output.NormalColor	 = float4(normal, 1.0f);
	output.MatColor		 = float4(roughness, metallic, material.glow, 0.0f);
	output.EmissiveColor = emissive;

	return output;
}