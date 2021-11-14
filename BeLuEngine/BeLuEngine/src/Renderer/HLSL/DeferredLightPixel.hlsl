#include "LightCalculations.hlsl"	// This includes "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float2 uv       : UV;
};

struct PS_OUTPUT
{
	float4 sceneColor: SV_TARGET0;
	float4 brightColor: SV_TARGET1;
};

PS_OUTPUT PS_main(VS_OUT input)
{
	// Sample from textures
	float2 uvScaled = float2(input.uv.x, input.uv.y);
	float4 albedo   = textures[cbPerScene.gBufferAlbedo].Sample(Anisotropic16_Wrap, uvScaled);
	float roughness = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uvScaled).r;
	float metallic  = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uvScaled).g;
	float glow		= textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uvScaled).b;
	float4 normal	= textures[cbPerScene.gBufferNormal].Sample(Anisotropic16_Wrap, uvScaled);
	float4 emissive = textures[cbPerScene.gBufferEmissive].Sample(Anisotropic16_Wrap, uvScaled);
	float4 reflData = textures[cbPerScene.reflectionSRV].Sample(Anisotropic16_Wrap, uvScaled);

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
	
		finalColor += CalcDirLight(
			dirLight,
			camPos,
			viewDir,
			worldPos,
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
			worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity,
			sceneBVH[cbPerScene.rayTracingBVH]);
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
	
	float3 ambient = float3(0.001f, 0.001f, 0.001f) * albedo;
	finalColor += ambient;

	PS_OUTPUT output = (PS_OUTPUT)0;

	output.sceneColor = float4(finalColor.rgb, 1.0f);

	if (glow == 1.0f)
	{
		output.brightColor = emissive;
	}
	else
	{
		output.brightColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	if (roughness > 0.95f)
	{
		output.sceneColor = float4(reflData.rgb, 1.0f);
	}

	return output;
}
