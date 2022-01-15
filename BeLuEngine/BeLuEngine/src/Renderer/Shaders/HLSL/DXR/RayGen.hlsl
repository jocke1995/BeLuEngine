#include "hlslhelpers.hlsl" // DXR Specifics
#include "../Common.hlsl"
#include "../DescriptorBindings.hlsl"

[shader("raygeneration")] 
void RayGen()
{
    unsigned int readIndex = rootConstantUints.index0;
    unsigned int writeIndex = rootConstantUints.index1;

	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	uint2 launchIndex = DispatchRaysIndex();
	
	float2 dims = float2(DispatchRaysDimensions().xy);

    // UV:s (0->1)
	float2 uv = launchIndex.xy / dims.xy;

    float4 albedo = textures[cbPerScene.gBufferAlbedo].SampleLevel(BilinearWrap, uv, 0);
	float depth   = textures[cbPerScene.depth].SampleLevel(BilinearWrap, uv, 0).r;
    float4 normal = textures[cbPerScene.gBufferNormal].SampleLevel(BilinearWrap, uv, 0);
    float roughness = textures[cbPerScene.gBufferMaterialProperties].SampleLevel(BilinearWrap, uv, 0).r;
    float metallic = textures[cbPerScene.gBufferMaterialProperties].SampleLevel(BilinearWrap, uv, 0).g;

	float3 worldPos = WorldPosFromDepth(depth, uv, cbPerFrame.projectionI, cbPerFrame.viewI);
    
    float3 cameraDir = normalize(worldPos - cbPerFrame.camPos);

    RayDesc ray;
    ray.Origin = float4(worldPos.xyz, 1.0f) + float4(normal.xyz, 0.0f) * 0.5f;
    ray.Direction = normalize(reflect(cameraDir, float3(normal.xyz)));
    ray.TMin = 0.01f;
    ray.TMax = 10000;
    
    // Initialize the ray payload
    ReflectionPayload reflectionPayload;
    reflectionPayload.color = float3(0.0f, 0.0f, 0.0f);
    reflectionPayload.recursionDepth = 0;

    // Trace the ray if it reflects anything
    float3 rayTracedColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    if (metallic > 0.1f)
    {
        rayTracedColor = TraceRadianceRay(ray, 0, sceneBVH[cbPerScene.rayTracingBVH]);
        finalColor = (textures[readIndex][launchIndex].rgb * (1 - metallic)) + (metallic * albedo * rayTracedColor);
    }
    else
    {
        finalColor = textures[readIndex][launchIndex].rgb;
    }

    texturesUAV[writeIndex][launchIndex] = float4(finalColor, 1.0f);
}
