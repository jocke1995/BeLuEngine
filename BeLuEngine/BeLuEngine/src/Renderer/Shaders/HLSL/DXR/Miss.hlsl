#include "hlslhelpers.hlsl" 

[shader("miss")]
void Miss(inout ReflectionPayload reflectionPayload : SV_RayPayload)
{
    unsigned int skyboxDescriptorIndex = cbPerScene.skybox;

    reflectionPayload.color = cubeTextures[skyboxDescriptorIndex].SampleLevel(BilinearWrap, WorldRayDirection(), 0).rgb;
}