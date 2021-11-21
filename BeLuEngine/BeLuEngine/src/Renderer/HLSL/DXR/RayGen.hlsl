#include "hlslhelpers.hlsl" // DXR Specifics
#include "../Common.hlsl"
#include "../DescriptorBindings.hlsl"


[shader("raygeneration")] 
void RayGen()
{
	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	uint2 launchIndex = DispatchRaysIndex();
	
	float2 dims = float2(DispatchRaysDimensions().xy);

    // UV:s (0->1)
	float2 uv = launchIndex.xy / dims.xy;

	float depth   = textures[cbPerScene.depth].SampleLevel(MIN_MAG_MIP_LINEAR_Wrap, uv, 0).r;
    float4 normal = textures[cbPerScene.gBufferNormal].SampleLevel(MIN_MAG_MIP_LINEAR_Wrap, uv, 0);
    float roughness = textures[cbPerScene.gBufferMaterialProperties].SampleLevel(MIN_MAG_MIP_LINEAR_Wrap, uv, 0).r;
    float metallic = textures[cbPerScene.gBufferMaterialProperties].SampleLevel(MIN_MAG_MIP_LINEAR_Wrap, uv, 0).g;

	float3 worldPos = WorldPosFromDepth(depth, uv, cbPerFrame.projectionI, cbPerFrame.viewI);
    
    float3 cameraDir = normalize(worldPos - cbPerFrame.camPos);

    RayDesc ray;
    ray.Origin = float4(worldPos.xyz, 1.0f) + float4(normal.xyz, 0.0f) * 0.5f;
    ray.Direction = normalize(reflect(cameraDir, float3(normal.xyz)));
    ray.TMin = 0;
    ray.TMax = 10000;
    
    // Initialize the ray payload
    ReflectionPayload reflectionPayload;
    reflectionPayload.color = float3(0.0f, 0.0f, 0.0f);
    reflectionPayload.recursionDepth = 0;
    
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    if (roughness < 0.11f && metallic > 0.95f)
    {
        // Trace the ray
        finalColor = TraceRadianceRay(ray, 0, sceneBVH[cbPerScene.rayTracingBVH]);
    }
    
    texturesUAV[cbPerScene.reflectionUAV][launchIndex] = float4(finalColor, 1.0f);
}
