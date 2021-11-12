#include "../../Headers/GPU_Structs.h"

SamplerState Anisotropic2_Wrap			: register (s0);
SamplerState Anisotropic4_Wrap			: register (s1);
SamplerState Anisotropic8_Wrap			: register (s2);
SamplerState Anisotropic16_Wrap			: register (s3);
SamplerState MIN_MAG_MIP_POINT_Border	: register (s4);
SamplerState MIN_MAG_MIP_LINEAR_Wrap	: register (s5);

Texture2D textures[]					   : register(t0, space1);
RaytracingAccelerationStructure SceneBVH[] : register(t0, space3);
RWTexture2D<float4> texturesUAV[]   : register(u0, space1);

ConstantBuffer<CB_PER_FRAME_STRUCT>  cbPerFrame	: register(b4, space0); // RootParam_CBV1
ConstantBuffer<CB_PER_SCENE_STRUCT>  cbPerScene : register(b5, space0); // RootParam_CBV2
