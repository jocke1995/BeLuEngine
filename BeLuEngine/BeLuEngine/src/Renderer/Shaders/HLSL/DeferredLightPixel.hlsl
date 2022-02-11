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
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo   = textures[cbPerScene.gBufferAlbedo].Sample(Anisotropic16_Wrap, uvScaled);
	float roughness = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uvScaled).r;
	float metallic  = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uvScaled).g;
	float4 normal	= textures[cbPerScene.gBufferNormal].Sample(Anisotropic16_Wrap, uvScaled);
	float4 emissive = textures[cbPerScene.gBufferEmissive].Sample(Anisotropic16_Wrap, uvScaled);

	float depthVal = textures[cbPerScene.depth].Sample(Anisotropic16_Wrap, uvScaled).r;
	float4 worldPos = float4(WorldPosFromDepth(depthVal, uvScaled, cbPerFrame.projectionI, cbPerFrame.viewI).xyz, 0.0f);
	float3 camPos = cbPerFrame.camPos;
	float3 finalColor = float3(0.0f, 0.0f, 0.0f);
	float3 viewDir = normalize(camPos - worldPos.xyz);
	
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
			worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);

		float3 position = float3(-dirLight.direction.xyz * 500);
		float3 lightDir = normalize(position - worldPos.xyz);
		float shadowFactor = RT_ShadowFactor(worldPos.xyz, 0.1f, 100000.0f, lightDir, sceneBVH[cbPerScene.rayTracingBVH]);

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
			worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);

		float3 lightDir = normalize(pointLight.position.xyz - worldPos.xyz);
		float shadowFactor = RT_ShadowFactor(worldPos.xyz, 0.1f, length(pointLight.position.xyz - worldPos.xyz) - 1.0, lightDir, sceneBVH[cbPerScene.rayTracingBVH]);

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
			worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);
	}
	
	float3 ambient = 0.03f * albedo;
	finalColor += ambient;

	PS_OUTPUT output = (PS_OUTPUT)0;

	output.sceneColor = float4(finalColor.rgb, 1.0f) + emissive;

	return output;
}
