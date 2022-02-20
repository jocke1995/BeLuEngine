#include "PBRMath.hlsl" // This includes Common.hlsl
#include "DescriptorBindings.hlsl"

float RT_ShadowFactor(float3 worldPos, float tMin, float tMax, float3 rayDir, RaytracingAccelerationStructure sceneBVH)
{
	RayQuery<RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
	
	uint rayFlags = 0;
	
	float shadowFactor = 1.0f;
	
	RayDesc ray = (RayDesc)0;
	ray.TMin = tMin;
	ray.TMax = tMax;
	
	ray.Direction = normalize(rayDir);
	ray.Origin = worldPos.xyz;
	
	q.TraceRayInline(
		sceneBVH,
		rayFlags,
		INSTANCE_MASK_SCENE_MINUS_NOSHADOWOBJECTS,
		ray
	);
	
	q.Proceed();
	
	if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		shadowFactor = 0.0f;
	}
	
	return shadowFactor;
}

float3 CalcDirLight(
	in DirectionalLight dirLight,
	in float3 camPos,
	in float3 viewDir,
	in float4 fragPos,
	in float metallic,
	in float3 albedo,
	in float roughness,
	in float3 normal,
	in float3 baseReflectivity)
{
	float3 DirLightContribution = float3(0.0f, 0.0f, 0.0f);

	float3 lightDir = normalize(-dirLight.direction.rgb);

	float3 halfwayVector = normalize(normalize(viewDir) + lightDir);

	float3 radiance = dirLight.baseLight.color.rgb * dirLight.baseLight.intensity;

	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0000001);
	float NdotL = max(dot(normal, lightDir), 0.0000001);
	float HdotV = max(dot(halfwayVector, viewDir), 0.0f);
	float HdotN = max(dot(halfwayVector, normal), 0.0f);

	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);

	float3 specular = D * G * F / (4.0f * NdotV * NdotL);

	// Energy conservation: ks + kd = 1
	float3 ks = F;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= (1.0f - metallic);

	DirLightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
	return DirLightContribution;
}

float3 CalcPointLight(
	in PointLight pointLight,
	in float3 camPos,
	in float3 viewDir,
	in float4 fragPos,
	in float metallic,
	in float3 albedo,
	in float roughness,
	in float3 normal,
	in float3 baseReflectivity)
{
	float3 pointLightContribution = float3(0.0f, 0.0f, 0.0f);
	float3 lightDir = normalize(pointLight.position.xyz - fragPos.xyz);

	float3 halfwayVector = normalize(viewDir + lightDir);

	// Attenuation
	float distancePixelToLight = length(pointLight.position.xyz - fragPos.xyz);
	float attenuation = 1.0f / pow(distancePixelToLight, 2);
	
	float3 radiance = pointLight.baseLight.color.rgb * pointLight.baseLight.intensity * attenuation;
	
	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0f);
	float NdotL = max(dot(normal, lightDir), 0.0f);
	float HdotV = max(dot(halfwayVector, viewDir), 0.0f);
	float HdotN = max(dot(halfwayVector, normal), 0.0f);
	
	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);
	
	float3 specular = D * G * F / (4.0f * NdotV * NdotL + 0.001f);
	
	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= (1.0f - metallic);
	
	pointLightContribution = (kD * albedo / PI + specular) * radiance * NdotL;
	return pointLightContribution;
}

float3 CalcSpotLight(
	in SpotLight spotLight,
	in float3 camPos,
	in float3 viewDir,
	in float4 fragPos,
	in float metallic,
	in float3 albedo,
	in float roughness,
	in float3 normal,
	in float3 baseReflectivity)
{
	float3 spotLightContribution = float3(0.0f, 0.0f, 0.0f);
	
	float3 lightDir = normalize(spotLight.position_cutOff.xyz - fragPos.xyz);
	float3 halfwayVector = normalize(normalize(viewDir) + lightDir);
	
	// Calculate the angle between lightdir and the direction of the light
	float theta = dot(lightDir, normalize(-spotLight.direction_outerCutoff.xyz));

	// To smooth edges
	float epsilon = (spotLight.position_cutOff.w - spotLight.direction_outerCutoff.w);
	float edgeIntensity = clamp((theta - spotLight.direction_outerCutoff.w) / epsilon, 0.0f, 1.0f);

	// Attenuation
	float distancePixelToLight = length(spotLight.position_cutOff.xyz - fragPos.xyz);
	float attenuation = 1.0f / pow(distancePixelToLight, 1);

	float3 radiance = spotLight.baseLight.color.rgb * spotLight.baseLight.intensity * attenuation;

	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0000001);
	float NdotL = max(dot(normal, lightDir), 0.0000001);
	float HdotV = max(dot(halfwayVector, viewDir), 0.0f);
	float HdotN = max(dot(halfwayVector, normal), 0.0f);

	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);

	float3 specular = D * G * F / (4.0f * NdotV * NdotL);

	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= (1.0f - metallic);

	spotLightContribution = ((kD * albedo / PI + specular) * radiance * NdotL) * edgeIntensity;
	return spotLightContribution;
}

float3 CalcRayTracedIBL(
	in float3 reflection,
	in float3 camPos,
	in float3 viewDir,
	in float metallic,
	in float3 albedo,
	in float roughness,
	in float3 normal,
	in float3 baseReflectivity)
{
	// Cook-Torrance BRDF

	// Since the IBL is treating the reflection as the light source, the lightDir is replaced with the reflection dir.
	float3 refl = reflect(-viewDir, normal);

	// Since the view and reflection directions are perpendicular, the halfwayDirection is equal to the Normal
	float3 halfwayVector = normal;

	float NdotV = max(dot(normal, viewDir), 1e-7);
	float NdotL = max(dot(normal, refl), 1e-7);
	float HdotV = max(dot(halfwayVector, viewDir), 1e-7);
	float HdotN = max(dot(halfwayVector, normal), 1e-7);

	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith_IBL(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect_Roughness(HdotV, baseReflectivity, roughness);

	float3 specularBRDF = D * G * F / max((4.0f * NdotV * NdotL), 1e-7);
	float specularPDF = D * HdotN / (4.0f * HdotV);

	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= (1.0f - metallic);

	// This would be the equivalent of sampling the irradiance cubemap using convoluted cupemaps as IBL
	float3 indirectDiffuseIrradiance = albedo;
	float3 diffuseBRDF = (indirectDiffuseIrradiance* kD * albedo) / PI;

	float3 specularContribution = reflection * F * specularBRDF / max(specularPDF, 1e-7);
	float3 diffuseContribution	= reflection * diffuseBRDF;

	return (diffuseContribution + specularContribution) * NdotL;
}