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

	float3 worldPos = WorldPosFromDepth(depth, uv, cbPerFrame.projectionI, cbPerFrame.viewI);
    
    float3 cameraDir = worldPos - cbPerFrame.camPos;

    RayDesc ray;
    ray.Origin = float4(worldPos.xyz, 1.0f);
    ray.Direction = reflect(cameraDir, float3(normal.xyz));
    ray.TMin = 1.0;
    ray.TMax = 10000;
    
    // Initialize the ray payload
    ReflectionPayload reflectionPayload;
    reflectionPayload.colorAndDistance = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    uint dhIndexBVH = cbPerScene.rayTracingBVH;
    
    // Trace the ray
    TraceRay(
        sceneBVH[dhIndexBVH],
        RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        0xFF,
        0,  // Hit group index
        0,
        0,  // Miss Shader index
        ray,
        reflectionPayload);

    //texturesUAV[cbPerScene.reflectionUAV][launchIndex] = float4(ReflectionPayload.colorAndDistance.rgb, 1.0f);
    texturesUAV[cbPerScene.reflectionUAV][launchIndex] = float4(reflectionPayload.colorAndDistance.rgb, 1.0f);
}
