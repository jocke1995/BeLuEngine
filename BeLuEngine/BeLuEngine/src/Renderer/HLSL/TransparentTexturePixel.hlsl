#include "LightCalculations.hlsl"	// This includes "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
	float3x3 tbn	: TBN;
};

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	MaterialData matData = globalRawBufferMaterial.Load<MaterialData>(slotInfo.materialIndex * sizeof(MaterialData));

	// Sample from textures
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo = textures[matData.textureAlbedo].Sample(Anisotropic16_Wrap, uvScaled);
	float roughness = matData.hasRoughnessTexture ? textures[matData.textureRoughness].Sample(Anisotropic16_Wrap, uvScaled).r : matData.roughnessValue;
	float metallic = matData.hasMetallicTexture ? textures[matData.textureMetallic].Sample(Anisotropic16_Wrap, uvScaled).r : matData.metallicValue;
	float4 emissive = matData.hasEmissiveTexture ? textures[matData.textureEmissive].Sample(Anisotropic16_Wrap, uvScaled) : matData.emissiveValue;
	float opacity   = matData.hasOpacityTexture ? textures[matData.textureOpacity].Sample(Anisotropic16_Wrap, uvScaled).r : matData.opacityValue;

	float3 normal = float3(0.0f, 0.0f, 0.0f);
	if (matData.hasNormalTexture)
	{
		normal = textures[matData.textureNormal].Sample(Anisotropic16_Wrap, uvScaled).xyz;
		normal = normalize((2.0f * normal) - 1.0f);
		normal = float4(normalize(mul(normal.xyz, input.tbn)), 0.0f);
	}
	else
	{
		normal = float4(input.norm.xyz, 0.0f);
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

		float3 lightColor = CalcDirLight(
			dirLight,
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);

		float3 position = float3(-dirLight.direction.xyz * 500);
		float3 lightDir = normalize(position - input.worldPos.xyz);
		float shadowFactor = RT_ShadowFactor(input.worldPos.xyz, 0.1f, 100000.0f, lightDir, sceneBVH[cbPerScene.rayTracingBVH]);

		finalColor += lightColor * shadowFactor;
	}

	// PointLight contributions
	for (unsigned int i = 0; i < lHeader.numPointLights; i++)
	{
		PointLight pointLight = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET + i * sizeof(PointLight));

		float3 lightColor = CalcPointLight(
			pointLight,
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);

		float3 lightDir = normalize(pointLight.position.xyz - input.worldPos.xyz);
		float shadowFactor = RT_ShadowFactor(input.worldPos.xyz, 0.1f, length(pointLight.position.xyz - input.worldPos.xyz) - 1.0, lightDir, sceneBVH[cbPerScene.rayTracingBVH]);

		finalColor += lightColor * shadowFactor;
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

	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo;
	finalColor += ambient;

	finalColor += emissive.rgb;

	return float4(finalColor.rgb, opacity);
}
