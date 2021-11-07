#include "ReflectionsCommon.hlsl"
#include "../Common.hlsl"
#include "hlslhelpers.hlsl"


// Raytracing acceleration structure, accessed as a SRV from a descriptorTable
RaytracingAccelerationStructure SceneBVH[] : register(t0, space3);

Texture2D textures[]   : register (t0, space1);
SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);

ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame		  : register(b4, space0);
ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene       : register(b5, space0);

[shader("raygeneration")] 
void RayGen()
{
	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	uint2 launchIndex = DispatchRaysIndex();
	
	float2 dims = float2(DispatchRaysDimensions().xy);

    // UV:s (0->1)
	float2 uv = launchIndex.xy / dims.xy;

	//float depth = textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).r;
    //
	//float3 worldPos = WorldPosFromDepth(depth, uv, cbPerFrame.projectionI, cbPerFrame.viewI);
    //
    //RayDesc ray;
    //ray.Origin = float4(worldPos.xyz, 1.0f);
    //ray.Direction = float3(0.0f, 1.0f, 1.0f);
    //ray.TMin = 1.0;
    //ray.TMax = distance(lightPos, worldPos);
    //
    //// Initialize the ray payload
    //ReflectionPayload ReflectionPayload;
    //ReflectionPayload.colorAndDistance = float4(1.0f, 1.0f, 0.0f, 1.0f);
    //
    //uint dhIndexBVH = cbPerScene.rayTracingBVH;
    //
    //// Trace the ray
    //TraceRay(
    //    SceneBVH[dhIndexBVH],
    //    RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
    //    0xFF,
    //    0,  // Hit group index
    //    0,
    //    0,  // Miss Shader index
    //    ray,
    //    ReflectionPayload);


}
