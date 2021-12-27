#include "../DescriptorBindings.hlsl"

static const int g_BlurRadius = 4;
static const int g_NumThreads = 64;
groupshared float4 g_SharedMem[g_NumThreads + 2 * g_BlurRadius];

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int readIndex = dhIndices.index0;
	unsigned int writeIndex = dhIndices.index1;
	unsigned int mipLevel = dhIndices.index2;

	uint width = 1280 >> mipLevel;
	uint height = 720 >> mipLevel;

	float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };
	/* -------------------- Clamp out of bound samples -------------------- */
	// left side
	if (groupThreadID.x < g_BlurRadius)
	{
		int x = max(dispatchThreadID.x - g_BlurRadius, 0);
		g_SharedMem[groupThreadID.x] = textures[readIndex].SampleLevel(BilinearClamp, int2(x, dispatchThreadID.y) / float2(width, height), 0);
	}

	// right side
	if (groupThreadID.x >= g_NumThreads - g_BlurRadius)
	{
		int x = min(dispatchThreadID.x + g_BlurRadius, width - 1);
		g_SharedMem[groupThreadID.x + 2 * g_BlurRadius] = textures[readIndex].SampleLevel(BilinearClamp, int2(x, dispatchThreadID.y) / float2(width, height), 0);
	}
	/* -------------------- Clamp out of bound samples -------------------- */

	// Fill the middle parts of the sharedMemory
	g_SharedMem[groupThreadID.x + g_BlurRadius] = textures[readIndex].SampleLevel(BilinearClamp, min(dispatchThreadID.xy / float2(width, height), float2(width, height) - 1), 0);

	// Wait for shared memory to be populated before reading from it
	GroupMemoryBarrierWithGroupSync();
	
	// Blur
	// Current fragments contribution
	float4 blurColor = weights[0] * g_SharedMem[groupThreadID.x + g_BlurRadius];

	// Adjacent fragment contributions
	for (int i = 1; i <= g_BlurRadius; i++)
	{
		int left = groupThreadID.x + g_BlurRadius - i;
		int right = groupThreadID.x + g_BlurRadius + i;
		blurColor += weights[i] * g_SharedMem[left];
		blurColor += weights[i] * g_SharedMem[right];
	}

	texturesUAV[writeIndex][dispatchThreadID.xy] = blurColor;
}