//#include "Common.hlsl" 
// 
//#include "hlslhelpers.hlsl" 
// 
// 
///* GLOBAL */ 
//StructuredBuffer<vertex> meshes[] : register(t0, space0); 
//StructuredBuffer<unsigned int> indices[] : register(t0, space1); 
//Texture2D textures[]   : register (t0, space2); 
// 
//// Raytracing acceleration structure, accessed as a SRV 
//RaytracingAccelerationStructure SceneBVH : register(t0, space4); 
// 
//SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5); 
// 
//ConstantBuffer<CB_PER_SCENE_STRUCT> cbPerScene : register(b5, space3); 
//ByteAddressBuffer rawBufferLights : register(t0, space3); 
// 
///* LOCAL */ 
//ConstantBuffer<DXR_WORLDMATRIX_STRUCT> worldMat : register(b7, space3); 
//ByteAddressBuffer rawBuffer: register(t0, space5); 
// 
//[shader("closesthit")]  
//void ClosestHit(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes attribs) 
//{ 
//    float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y); 
// 
//    SlotInfo info = rawBuffer.Load<SlotInfo>(GeometryIndex() * sizeof(SlotInfo)); 
// 
//    uint vertId = 3 * PrimitiveIndex(); 
//    uint IndexID1 = indices[info.indicesIndex][vertId + 0]; 
//    uint IndexID2 = indices[info.indicesIndex][vertId + 1]; 
//    uint IndexID3 = indices[info.indicesIndex][vertId + 2]; 
// 
//    // Get the normal 
//    vertex v1 = meshes[info.vertexDataIndex][IndexID1]; 
//    vertex v2 = meshes[info.vertexDataIndex][IndexID2]; 
//    vertex v3 = meshes[info.vertexDataIndex][IndexID3]; 
// 
//    float3 norm = v1.norm * bary.x + v2.norm * bary.y + v3.norm * bary.z; 
//    float3 normal = normalize(mul(float4(norm, 0.0f), worldMat.worldMatrix)); 
// 
//    float2 uv = v1.uv * bary.x + v2.uv * bary.y + v3.uv * bary.z; 
//    
//    float3 albedo = textures[info.textureAlbedo].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).rgb; 
//    float3 materialColor = float3(0.5f, 0.5f, 0.5f); 
//    materialColor =  albedo.rgb; 
//    float3 finalColor = float3(0.0f, 0.0f, 0.0f); 
//	 
//	 
//	uint2 launchIndex = DispatchRaysIndex(); 
//	float2 dims = float2(DispatchRaysDimensions().xy); 
//	// From 0 to 1 ----> -1 to 1 
//	float2 d = ((launchIndex.xy + 0.5f) / dims.xy); 
//	 
//	 
//	 
//	float lightRadius = 1.0; // To low radius => coneAngle not accurate enough 
//    float3 worldOrigin = WorldRayOrigin() + RayTCurrent() * WorldRayDirection(); 
//    //float3 lightPos = float3(-26.42f, 63.0776f, 14.19f); 
// 
// 
//    // PointLight Test 
//    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0); 
//    for (int i = 0; i < lHeader.numLights; i++) 
//    { 
//        PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight)); 
//        float3 lightPos = pl.position.xyz; 
//        float3 lightColor = pl.baseLight.color; 
//        // Find the world - space hit position 
//        float3 lightDir = normalize(lightPos - worldOrigin); 
// 
//        float3 perpL = cross(lightDir, float3(0.f, 1.0f, 0.f)); 
//        // Handle case where L = up -> perpL should then be (1,0,0) 
//        if (all(perpL == 0.0f)) 
//        { 
//            perpL.x = 1.0; 
//        } 
// 
//        // Use perpL to get a vector from worldPosition to the edge of the light sphere 
//        float3 toLightEdge = normalize((lightPos + perpL * lightRadius) - worldOrigin); 
// 
//        // Angle between L and toLightEdge. Used as the cone angle when sampling shadow rays 
//        float coneAngle = acos(dot(lightDir, toLightEdge)) * 2; 
// 
//        // Init random floats 
//        uint frameSeed = cbPerFrame.frameCounter + 200000; 
//        uint seed = initRand(frameSeed * d.x, frameSeed * d.y); 
// 
//        float sumFactor = 0; 
//        int spp = 1; 
// 
//        for (int j = 0; j < spp; j++) 
//        { 
//            sumFactor = 0; 
//            float3 randDir = getConeSample(seed, lightDir, coneAngle); 
// 
//            // Fire a shadow ray. The direction is hard-coded here, but can be fetched 
//            // from a constant-buffer 
//            RayDesc ray; 
//            ray.Origin = worldOrigin; 
//            ray.Direction = randDir; 
//            ray.TMin = 0.01; 
//            ray.TMax = distance(lightPos, ray.Origin); 
// 
//            // Initialize the ray payload 
//            ShadowHitInfo shadowPayload; 
//            shadowPayload.isHit = true; 
// 
//            // Trace the ray 
//            TraceRay( 
//                // Acceleration structure 
//                SceneBVH, 
//                // Flags can be used to specify the behavior upon hitting a surface 
//                //RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 
//                RAY_FLAG_NONE, 
//                // Instance inclusion mask, which can be used to mask out some geometry to 
//                // this ray by and-ing the mask with a geometry mask. The 0xFF flag then 
//                // indicates no geometry will be masked 
//                0xFF, 
//                // Depending on the type of ray, a given object can have several hit 
//                // groups attached (ie. what to do when hitting to compute regular 
//                // shading, and what to do when hitting to compute shadows). Those hit 
//                // groups are specified sequentially in the SBT, so the value below 
//                // indicates which offset (on 4 bits) to apply to the hit groups for this 
//                // ray. In this sample we only have one hit group per object, hence an 
//                // offset of 0. 
//                1, 
//                // The offsets in the SBT can be computed from the object ID, its instance 
//                // ID, but also simply by the order the objects have been pushed in the 
//                // acceleration structure. This allows the application to group shaders in 
//                // the SBT in the same order as they are added in the AS, in which case 
//                // the value below represents the stride (4 bits representing the number 
//                // of hit groups) between two consecutive objects. 
//                0, 
//                // Index of the miss shader to use in case several consecutive miss 
//                // shaders are present in the SBT. This allows to change the behavior of 
//                // the program when no geometry have been hit, for example one to return a 
//                // sky color for regular rendering, and another returning a full 
//                // visibility value for shadow rays. This sample has only one miss shader, 
//                // hence an index 0 
//                1, 
//                // Ray information to trace 
//                ray, 
//                // Payload associated to the ray, which will be used to communicate 
//                // between the hit/miss shaders and the raygen 
//                shadowPayload); 
// 
//            float factor = shadowPayload.isHit ? 0.0 : 1.0; 
//            sumFactor += factor; 
//        } 
// 
//        sumFactor /= spp; 
//        float nDotL = max(0.0f, dot(normal, lightDir)); 
// 
//        finalColor += materialColor * lightColor * sumFactor * nDotL; 
//    } 
//     
//   
//    float3 ambient = materialColor * float3(0.1f, 0.1f, 0.1f); 
//    payload.colorAndDistance = float4(finalColor + ambient, RayTCurrent()); 
//} 