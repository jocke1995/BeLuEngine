#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 256;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int bloomTextureIndex = dhIndices.index0;
	unsigned int sceneTextureIndex = dhIndices.index1;
	unsigned int combinedUAVTextureIndex = dhIndices.index2;

	float4 bloomColor = textures[bloomTextureIndex][dispatchThreadID.xy];
	float4 sceneColor = textures[sceneTextureIndex][dispatchThreadID.xy];

	float4 finalColor = bloomColor + sceneColor;

	texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = finalColor;
}