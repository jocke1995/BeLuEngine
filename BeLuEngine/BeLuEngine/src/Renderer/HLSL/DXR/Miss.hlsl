#include "hlslhelpers.hlsl" 

[shader("miss")]
void Miss(inout ReflectionPayload reflectionPayload : SV_RayPayload)
{
    reflectionPayload.color = float3(0.0f, 0.0f, 0.0f);
}