#include "hlslhelpers.hlsl"
#include "../Common.hlsl"
#include "../DescriptorBindings.hlsl"
 
/* GLOBAL */ 
//StructuredBuffer<vertex> meshes[] : register(t0, space1); 
//StructuredBuffer<unsigned int> indices[] : register(t0, space2); 
 
/* LOCAL */ 
//ByteAddressBuffer rawBuffer: register(t6, space0);
 
[shader("closesthit")]  
void ClosestHit(inout ReflectionPayload reflectionPayload, in BuiltInTriangleIntersectionAttributes attribs)
{ 
    //float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y); 
    //
    //SlotInfo info = rawBuffer.Load<SlotInfo>(GeometryIndex() * sizeof(SlotInfo)); 
    //
    //uint vertId = 3 * PrimitiveIndex(); 
    //uint IndexID1 = indices[info.indicesIndex][vertId + 0]; 
    //uint IndexID2 = indices[info.indicesIndex][vertId + 1]; 
    //uint IndexID3 = indices[info.indicesIndex][vertId + 2]; 
    //
    //// Get the normal 
    //vertex v1 = meshes[info.vertexDataIndex][IndexID1]; 
    //vertex v2 = meshes[info.vertexDataIndex][IndexID2]; 
    //vertex v3 = meshes[info.vertexDataIndex][IndexID3]; 
    //
    //float3 norm = v1.norm * bary.x + v2.norm * bary.y + v3.norm * bary.z; 
    //float3 normal = normalize(mul(float4(norm, 0.0f), worldMat.worldMatrix)); 
    //
    //float2 uv = v1.uv * bary.x + v2.uv * bary.y + v3.uv * bary.z; 
    //
    //float3 albedo = textures[info.textureAlbedo].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).rgb; 
    //float3 materialColor = float3(0.5f, 0.5f, 0.5f); 
    //materialColor =  albedo.rgb; 
    //float3 finalColor = float3(0.0f, 0.0f, 0.0f); 
	// 
	// 
	//uint2 launchIndex = DispatchRaysIndex(); 
	//float2 dims = float2(DispatchRaysDimensions().xy); 
	//// From 0 to 1 ----> -1 to 1 
	//float2 d = ((launchIndex.xy + 0.5f) / dims.xy); 
	// 
    //float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection(); 
    ////float3 lightPos = float3(-26.42f, 63.0776f, 14.19f); 
    //
    //
    //
    //float3 ambient = materialColor * float3(0.1f, 0.1f, 0.1f); 
    //payload.colorAndDistance = float4(finalColor + ambient, RayTCurrent()); 
    reflectionPayload.colorAndDistance = float4(0.0f, 1.0f, 0.0f, 1.0f);
} 