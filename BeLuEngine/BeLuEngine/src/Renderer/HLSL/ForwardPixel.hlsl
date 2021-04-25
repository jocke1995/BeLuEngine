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

ConstantBuffer<DirectionalLight> dirLight[]	: register(b0, space0);
ConstantBuffer<PointLight> pointLight[]		: register(b0, space1);
ConstantBuffer<SpotLight> spotLight[]		: register(b0, space2);

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

	// DirectionalLight contributions
	for (unsigned int i = 0; i < cbPerScene.Num_Dir_Lights; i++)
	{
		int index = cbPerScene.dirLightIndices[i].x;
	
		//DirectionalLight asd = rawBufferLights[19].Load<DirectionalLight>(sizeof(LightHeader));
		DirectionalLight dirLight = rawBufferLights.Load<DirectionalLight>(sizeof(LightHeader));


		//DirectionalLight dl;
		//dl.direction = float4(-1.0f, -2.0f, 0.03f, 0.0f);
		//dl.baseLight.color = float3(1.0f, 1.0f, 1.0f);
		//dl.baseLight.intensity = 3.0f;
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
/*for (unsigned int i = 0; i < cbPerScene.Num_Point_Lights; i++)
	{
		int index = cbPerScene.pointLightIndices[i].x;

		finalColor += CalcPointLight(
			pointLight[index],
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
	for (unsigned int i = 0; i < cbPerScene.Num_Spot_Lights; i++)
	{
		int index = cbPerScene.spotLightIndices[i].x;
	
		finalColor += CalcSpotLight(
			spotLight[index],
			camPos,
			viewDir,
			input.worldPos,
			metallic,
			albedo.rgb,
			roughness,
			normal.rgb,
			baseReflectivity);
	}*/
	
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
