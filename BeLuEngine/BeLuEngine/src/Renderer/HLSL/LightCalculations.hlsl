#include "PBRMath.hlsl" // This includes Common.hlsl
#include "DescriptorBindings.hlsl"

float RT_ShadowFactor(float3 worldPos, float tMin, float tMax, float3 rayDir, RaytracingAccelerationStructure sceneBVH)
{
	RayQuery<RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> q;
	
	uint rayFlags = 0;
	uint instanceMask = 0xff;
	
	float shadowFactor = 1.0f;
	
	RayDesc ray = (RayDesc)0;
	ray.TMin = tMin;
	ray.TMax = tMax;
	
	ray.Direction = normalize(rayDir);
	ray.Origin = worldPos.xyz;
	
	q.TraceRayInline(
		sceneBVH,
		rayFlags,
		instanceMask,
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

	float3 normalized_bisector = normalize(normalize(viewDir) + lightDir);

	float3 radiance = dirLight.baseLight.color.rgb * dirLight.baseLight.intensity;

	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0000001);
	float NdotL = max(dot(normal, lightDir), 0.0000001);
	float HdotV = max(dot(normalized_bisector, viewDir), 0.0f);
	float HdotN = max(dot(normalized_bisector, normal), 0.0f);

	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);

	float3 specular = D * G * F / (4.0f * NdotV * NdotL);

	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= 1.0f - metallic;

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

	float3 normalized_bisector = normalize(viewDir + lightDir);

	// Attenuation
	float constantFactor = pointLight.attenuation.x;
	float linearFactor = pointLight.attenuation.y;
	float quadraticFactor = pointLight.attenuation.z;
	float distancePixelToLight = length(pointLight.position.xyz - fragPos.xyz);
	float attenuation = 1.0f / (constantFactor + (linearFactor * distancePixelToLight) + (quadraticFactor * pow(distancePixelToLight, 2)));
	
	float3 radiance = pointLight.baseLight.color.rgb * pointLight.baseLight.intensity * attenuation;
	
	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0f);
	float NdotL = max(dot(normal, lightDir), 0.0f);
	float HdotV = max(dot(normalized_bisector, viewDir), 0.0f);
	float HdotN = max(dot(normalized_bisector, normal), 0.0f);
	
	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);
	
	float3 specular = D * G * F / (4.0f * NdotV * NdotL + 0.001f);
	
	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= 1.0f - metallic;
	
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
	float3 normalized_bisector = normalize(normalize(viewDir) + lightDir);
	
	// Calculate the angle between lightdir and the direction of the light
	float theta = dot(lightDir, normalize(-spotLight.direction_outerCutoff.xyz));

	// To smooth edges
	float epsilon = (spotLight.position_cutOff.w - spotLight.direction_outerCutoff.w);
	float edgeIntensity = clamp((theta - spotLight.direction_outerCutoff.w) / epsilon, 0.0f, 1.0f);

	// Attenuation
	float constantFactor = spotLight.attenuation.x;
	float linearFactor = spotLight.attenuation.y;
	float quadraticFactor = spotLight.attenuation.z;
	float distancePixelToLight = length(spotLight.position_cutOff.xyz - fragPos.xyz);
	float attenuation = 1.0f / (constantFactor + (linearFactor * distancePixelToLight) + (quadraticFactor * pow(distancePixelToLight, 2)));

	float3 radiance = spotLight.baseLight.color.rgb * spotLight.baseLight.intensity * attenuation;

	// Cook-Torrance BRDF
	float NdotV = max(dot(normal, viewDir), 0.0000001);
	float NdotL = max(dot(normal, lightDir), 0.0000001);
	float HdotV = max(dot(normalized_bisector, viewDir), 0.0f);
	float HdotN = max(dot(normalized_bisector, normal), 0.0f);

	float  D = NormalDistributionGGX(HdotN, roughness);
	float  G = GeometrySmith(NdotV, NdotL, roughness);
	float3 F = CalculateFresnelEffect(HdotV, baseReflectivity);

	float3 specular = D * G * F / (4.0f * NdotV * NdotL);

	// Energy conservation
	float3 kD = float3(1.0f, 1.0f, 1.0f) - F;
	kD *= 1.0f - metallic;

	spotLightContribution = ((kD * albedo / PI + specular) * radiance * NdotL) * edgeIntensity;
	return spotLightContribution;
}
