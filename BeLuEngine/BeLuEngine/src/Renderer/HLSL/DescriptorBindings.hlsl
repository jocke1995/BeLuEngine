#include "../../Headers/GPU_Structs.h"

// Static Samplers
SamplerState Anisotropic2_Wrap			: register (s0);
SamplerState Anisotropic4_Wrap			: register (s1);
SamplerState Anisotropic8_Wrap			: register (s2);
SamplerState Anisotropic16_Wrap			: register (s3);
SamplerState MIN_MAG_MIP_POINT_Border	: register (s4);
SamplerState MIN_MAG_MIP_LINEAR_Wrap	: register (s5);

// SRV table with 4 ranges, all starting at 0 in the dHeap
Texture2D						textures[]	: register(t0, space1);
StructuredBuffer<vertex>		meshes[]	: register(t0, space2);
StructuredBuffer<unsigned int>	indices[]	: register(t0, space2);
RaytracingAccelerationStructure sceneBVH[]	: register(t0, space4);

// UAV table with 3 ranges, all starting at 0 in the dHeap
RWTexture2D<float4> texturesUAV[]   : register(u0, space1);


// CBVs
ConstantBuffer<SlotInfo>					slotInfo			: register(b0, space0);
ConstantBuffer<DescriptorHeapIndices>		dhIndices			: register(b1, space0);
ConstantBuffer<MATRICES_PER_OBJECT_STRUCT>	matricesPerObject	: register(b2, space0);
ConstantBuffer<CB_PER_FRAME_STRUCT>			cbPerFrame			: register(b3, space0);
ConstantBuffer<CB_PER_SCENE_STRUCT>			cbPerScene			: register(b4, space0);
ConstantBuffer<MaterialData>				material			: register(b5, space0);

// SRVs
ByteAddressBuffer rawBufferLights: register(t0, space0);

// UAVs (not yet used, will be used for readWrite buffers later, maybe when using ExecuteIndirect)