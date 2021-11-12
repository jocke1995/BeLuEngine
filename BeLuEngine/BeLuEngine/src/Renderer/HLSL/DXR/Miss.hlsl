#include "hlslhelpers.hlsl" 

[shader("miss")]
void Miss(inout ReflectionPayload reflectionPayload : SV_RayPayload)
{
    reflectionPayload.colorAndDistance = float4(1.0f, 0.0f, 0.0f, 0.0f);
}