#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;


[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int bloomTextureIndex = rootConstantUints.index0;
	unsigned int sceneTextureIndex = rootConstantUints.index1;
	unsigned int mipLevel = rootConstantUints.index2;

	float4 bloomColor = textures[bloomTextureIndex][dispatchThreadID.xy];

	// Additive blend
	texturesUAV[sceneTextureIndex][dispatchThreadID.xy] += bloomColor;
}