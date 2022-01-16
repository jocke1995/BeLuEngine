#include "hlslhelpers.hlsl" 

[shader("miss")]
void ShadowMiss(inout ShadowPayload shadowPayload : SV_RayPayload)
{
    shadowPayload.isHit = false;
}