#include "Common.hlsl"
// Source: https://www.youtube.com/watch?v=5p0e7YNONr8&ab_channel=BrianWill

// Proportion of specular reflectance
float3 CalculateFresnelEffect(float HdotV, float3 F0) // F0 == baseReflectivity
{
	float3 iVec = float3(1.0f, 1.0f, 1.0f);

	return F0 + (iVec - F0) * pow(iVec - HdotV, 5.0f);
}

// Proportion of specular reflectance (used for IBL calculations)
float3 CalculateFresnelEffect_Roughness(float HdotV, float3 F0, float roughness)
{
	float3 iVec = float3(1.0f, 1.0f, 1.0f);

	float temp = float3(1.0f, 1.0f, 1.0f) - float3(roughness, roughness, roughness);
	return F0 + (max(temp, F0) - F0) * pow(saturate(iVec - HdotV), 5.0f);
}

// Approximate the specular reflectivity based on the roughness of the material
// In this case, we use Trowbridge-Reitz GGX normal distribution function
float NormalDistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	
	float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
	denom = PI * denom * denom;
	return a2 / max(denom, 0.00000001f);
}

// Approximate the self-shadowing for each microsurface
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	float k = pow(roughness + 1.0f, 2) / 8.0f;

	float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
	float ggx2 = NdotL / (NdotL * (1.0f - k) + k);

	return ggx1 * ggx2;
}

// Approximate the self-shadowing for each microsurface (used for IBL calculations)
float GeometrySmith_IBL(float NdotV, float NdotL, float roughness)
{
	float k = pow(roughness, 2) / 2;

	float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
	float ggx2 = NdotL / (NdotL * (1.0f - k) + k);

	return ggx1 * ggx2;
}