struct vertex
{
    float3 pos;
    float2 uv;
    float3 norm;
    float3 tang;
};

// Hit information, aka ray payload
struct ReflectionPayload
{
    float4 colorAndDistance;
};

// Attributes output by the raytracing when hitting a surface,
// here the barycentric coordinates
struct Attributes
{
    float2 bary;
};
