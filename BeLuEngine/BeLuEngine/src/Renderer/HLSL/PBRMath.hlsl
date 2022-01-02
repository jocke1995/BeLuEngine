#include "Common.hlsl"
// Source: https://www.youtube.com/watch?v=5p0e7YNONr8&ab_channel=BrianWill

// Proportion of specular reflectance
float3 CalculateFresnelEffect(float HdotV, float3 baseReflectivity)
{
	float3 IdentityVector = float3(1.0f, 1.0f, 1.0f);

	return baseReflectivity + (IdentityVector - baseReflectivity) * pow(IdentityVector - HdotV, 5.0f);
}

// Approximate the specular reflectivity based on the roughness of the material
// In this case, we use Trowbridge-Reitz GGX normal distribution function
float NormalDistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
	denom = PI * denom * denom;
	return a / max(denom, 0.00000001f);
}

// Approximate the self-shadowing for each microsurface
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float k = pow(roughness + 1.0f, 2) / 8.0f;

	float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
	float ggx2 = NdotL / (NdotL * (1.0f - k) + k);

	return ggx1 * ggx2;
}