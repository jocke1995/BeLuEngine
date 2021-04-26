#include "LightCalculations.hlsl"

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
	float4 sceneColor: SV_TARGET0;
	float4 brightColor: SV_TARGET1;
};

ByteAddressBuffer rawBufferLights: register(t0, space4);
//ByteAddressBuffer rawBufferLights[]: register(t0, space1); // TODO: not working to put rawBuffer in descriptorTable?

ConstantBuffer<SlotInfo> info					 : register(b1, space3);
ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame  : register(b4, space3);
ConstantBuffer<MaterialData> material			 : register(b6, space3);

PS_OUTPUT PS_main(VS_OUT input)
{
	// Sample from textures
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo   = textures[material.textureAlbedo].Sample(Anisotropic16_Wrap, uvScaled);
	float roughness = material.hasRoughnessTexture ? textures[material.textureRoughness].Sample(Anisotropic16_Wrap, uvScaled).r : material.roughnessValue;
	float metallic  = material.hasMetallicTexture ? textures[material.textureMetallic].Sample(Anisotropic16_Wrap, uvScaled).r : material.metallicValue;
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

	float3 camPos = cbPerFrame.camPos;
	float3 finalColor = float3(0.0f, 0.0f, 0.0f);
	float3 viewDir = normalize(camPos - input.worldPos.xyz);

	// Linear interpolation
	float3 baseReflectivity = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, metallic);

	LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
	// DirectionalLight contributions
	for (unsigned int i = 0; i < lHeader.numDirectionalLights; i++)
	{
		DirectionalLight dirLight = rawBufferLights.Load<DirectionalLight>(sizeof(LightHeader) + i * sizeof(DirectionalLight));

		finalColor += CalcDirLight(
			dirLight,
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);
	}

	// PointLight contributions
	for (unsigned int i = 0; i < lHeader.numPointLights; i++)
	{
		PointLight pointLight = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET + i * sizeof(PointLight));

		finalColor += CalcPointLight(
			pointLight,
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);
	}

	// SpotLight  contributions
	for (unsigned int i = 0; i < lHeader.numSpotLights; i++)
	{
		SpotLight spotLight = rawBufferLights.Load<SpotLight>(sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET + POINT_LIGHT_MAXOFFSET + i * sizeof(SpotLight));
	
		finalColor += CalcSpotLight(
			spotLight,
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);
	}
	
	float3 ambient = float3(0.001f, 0.001f, 0.001f) * albedo;
	finalColor += ambient;

	finalColor += emissive.rgb * emissive.a;

	PS_OUTPUT output = (PS_OUTPUT)0;

	output.sceneColor = float4(finalColor.rgb, 1.0f);

	float brightness = dot(output.sceneColor.rgb, float3(0.2126, 0.7152, 0.0722));
	if (brightness > 1.0f && material.glow == true)
	{
		output.brightColor = output.sceneColor;
	}
	else
	{
		output.brightColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
	return output;
}
