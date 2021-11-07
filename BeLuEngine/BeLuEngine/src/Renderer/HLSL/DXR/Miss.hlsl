#include "Common.hlsl"

RWTexture2D<float4> light_uav[] : register(u0, space1);
ByteAddressBuffer rawBufferLights: register(t0, space3);

[shader("miss")]
void Miss(inout HitInfo payload : SV_RayPayload)
{
    // Set all light buffers to 1 (In light)
    LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
    for (int i = 0; i < lHeader.numLights; i++)
    {
        light_uav[i * 2 + 1][DispatchRaysIndex().xy] = float4(1, 1, 1, 1);
    }

    payload.colorAndDistance = float4(0.2f, 0.2f, 0.8f, -1.f);
}