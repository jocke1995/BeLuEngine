#include "../DescriptorBindings.hlsl"

// Hit information, aka ray payload
struct ReflectionPayload
{
    float3 color;
    unsigned int recursionDepth;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
    float2 bary;
};

float3 TraceRadianceRay(in RayDesc rayDescIn, in unsigned int currentRayRecursionDepth, RaytracingAccelerationStructure sceneBVH)
{
    if (currentRayRecursionDepth >= 2)
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
        0xFF,
        0,  // Hit group index
        0,
        0,  // Miss Shader index
        ray,
        rayPayload);

    return rayPayload.color;
}