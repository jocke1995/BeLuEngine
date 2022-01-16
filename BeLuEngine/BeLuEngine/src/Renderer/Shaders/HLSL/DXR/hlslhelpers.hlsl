#include "../DescriptorBindings.hlsl"

// Hit information, aka ray payload
struct ReflectionPayload
{
    float3 color;
    unsigned int recursionDepth;
};

struct ShadowPayload
{
    bool isHit;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
    float2 bary;
};

float3 TraceRadianceRay(in RayDesc rayDescIn, in unsigned int currentRayRecursionDepth, RaytracingAccelerationStructure sceneBVH)
{
    if (currentRayRecursionDepth >= 4)
    {
        return float3(0, 0, 0);
    }

    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = rayDescIn.Origin;
    ray.Direction = rayDescIn.Direction;
    ray.TMin = 0.01f;
    ray.TMax = 10000;

    ReflectionPayload rayPayload;
    rayPayload.color = float3(0.0f, 0.0f, 0.0f);
    rayPayload.recursionDepth = currentRayRecursionDepth + 1;

    TraceRay(
        sceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
        INSTANCE_MASK_ENTIRE_SCENE,
        0,  // Hit group index
        0,
        MISS_SHADER_REFLECTIONS,  // Miss Shader index
        ray,
        rayPayload);

    return rayPayload.color;
}

bool TraceShadowRay(in RayDesc shadowRayDescIn, RaytracingAccelerationStructure sceneBVH)
{
    ShadowPayload shadowPayload;
    shadowPayload.isHit = true;

    TraceRay(
        sceneBVH,
        RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
        INSTANCE_MASK_SCENE_MINUS_NOSHADOWOBJECTS,
        0,  // Hit group index
        0,
        MISS_SHADER_SHADOWS,  // Miss Shader index
        shadowRayDescIn,
        shadowPayload);

    return shadowPayload.isHit;
}