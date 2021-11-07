#include "Common.hlsl"
#include "hlslhelpers.hlsl"

// Raytracing acceleration structure, accessed as a SRV
RaytracingAccelerationStructure SceneBVH : register(t0, space4);

Texture2D textures[]   : register (t0, space2);
SamplerState MIN_MAG_MIP_LINEAR__WRAP : register(s5);
RWTexture2D<float4> light_uav[] : register(u0, space1);

ConstantBuffer<CB_PER_OBJECT_STRUCT> cbPerObject	  : register(b1, space3);
ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame		  : register(b4, space3);
ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene       : register(b5, space3);
ConstantBuffer<DXR_CAMERA>           cbCameraMatrices : register(b6, space3);

ByteAddressBuffer rawBufferLights : register(t0, space3);

// Calculate world pos from DepthBuffer
float3 WorldPosFromDepth(float depth, float2 TexCoord)
{
    TexCoord.y = 1.0 - TexCoord.y;
    float4 clipSpacePosition = float4(TexCoord * 2.0 - 1.0, depth, 1.0);
    float4 viewSpacePosition = mul(cbCameraMatrices.projectionI, clipSpacePosition);

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    float4 worldSpacePosition = mul(cbCameraMatrices.viewI, viewSpacePosition);

    return worldSpacePosition.xyz;
}

[shader("raygeneration")] 
void RayGen()
{
	// Get the location within the dispatched 2D grid of work items
	// (often maps to pixels, so this could represent a pixel coordinate).
	uint2 launchIndex = DispatchRaysIndex();
	
	float2 dims = float2(DispatchRaysDimensions().xy);

    // UV:s (0->1)
	float2 uv = launchIndex.xy / dims.xy;

	float depth = textures[cbPerScene.depthBufferIndex].SampleLevel(MIN_MAG_MIP_LINEAR__WRAP, uv, 0).r;

	float3 worldPos = WorldPosFromDepth(depth, uv);

    // Init random floats
    uint frameSeed = cbPerFrame.frameCounter + 200000;
    uint seed = initRand(frameSeed * uv.x, frameSeed * uv.y);

    // PointLight Test
    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
    for (int i = 0; i < lHeader.numLights; i++)
    {
        PointLight pl = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + i * sizeof(PointLight));
        float3 lightPos = pl.position.xyz;
        float3 lightColor = pl.baseLight.color;

        // Find the world - space hit position
        float3 lightDir = normalize(lightPos - worldPos);

        // Maybe have this attribute inside pointlight?
        float lightRadius = 1.0; // To low radius => coneAngle not accurate enough
    
        float3 perpL = normalize(cross(lightDir, float3(0.f, 1.0f, 0.f)));
        // Handle case where L = up -> perpL should then be (1,0,0)
        if (all(perpL == 0.0f))
        {
            perpL.x = 1.0;
        }
    
        // Use perpL to get a vector from worldPosition to the edge of the light sphere
        float3 toLightEdge = normalize((lightPos + perpL * lightRadius) - worldPos);
    
        // Angle between L and toLightEdge. Used as the cone angle when sampling shadow rays
        float coneAngle = acos(dot(lightDir, toLightEdge)) * 2;
    
        
    
        float sumFactor = 0;
 
        for (int j = 0; j < cbPerScene.spp; j++)
        {
            float factor = 0;
            float3 randDir = getConeSample(seed, lightDir, coneAngle);

            RayDesc ray;
            ray.Origin = float4(worldPos.xyz, 1.0f);
            ray.Direction = normalize(randDir);
            ray.TMin = 1.0;
            ray.TMax = distance(lightPos, worldPos);

            // Initialize the ray payload
            ShadowHitInfo shadowPayload;
            shadowPayload.isHit = true;

            // Trace the ray
            TraceRay(
                SceneBVH,
                RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
                0xFF,
                0,  // Hit group index
                0,
                0,  // Miss Shader index
                ray,
                shadowPayload);

            factor = shadowPayload.isHit ? 0.0 : 1.0;
            sumFactor += factor;
        }

        sumFactor /= cbPerScene.spp;
        // i * 2 + 1 - PingPongResource has (0)SRV (1)UAV.
        light_uav[i * 2 + 1][DispatchRaysIndex().xy] = min(sumFactor, 1.0);
    }
}
