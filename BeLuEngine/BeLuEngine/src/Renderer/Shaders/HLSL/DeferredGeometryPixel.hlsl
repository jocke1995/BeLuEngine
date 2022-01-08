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

	MaterialData matData = globalRawBufferMaterial.Load<MaterialData>(slotInfo.materialIndex * sizeof(MaterialData));

	// Sample from textures
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo = matData.hasAlbedoTexture ? textures[matData.textureAlbedo].Sample(Anisotropic16_Wrap, uvScaled) : matData.albedoValue;
	float roughness = matData.hasRoughnessTexture ? textures[matData.textureRoughness].Sample(Anisotropic16_Wrap, uvScaled).r : matData.roughnessValue;
	float metallic = matData.hasMetallicTexture ? textures[matData.textureMetallic].Sample(Anisotropic16_Wrap, uvScaled).r : matData.metallicValue;
	float4 emissive = matData.hasEmissiveTexture ? textures[matData.textureEmissive].Sample(Anisotropic16_Wrap, uvScaled) : matData.emissiveValue;
	
	float3 normal = float3(0.0f, 0.0f, 0.0f);
	if (matData.hasNormalTexture)
	{
		normal = textures[matData.textureNormal].Sample(Anisotropic16_Wrap, uvScaled).xyz;
		normal = normalize((2.0f * normal) - 1.0f);
		normal = normalize(mul(normal, input.tbn));
	}
	else
	{
		normal = input.norm.xyz;
	}

	output.AlbedoColor	 = albedo;
	output.NormalColor	 = float4(normal, 1.0f);
	output.MatColor		 = float4(roughness, metallic, 0.0f, 0.0f);
	output.EmissiveColor = float4(emissive.rgb * emissive.a, 1.0f);

	return output;
}