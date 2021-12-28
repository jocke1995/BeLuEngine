#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int downScaledTextureIndex = dhIndices.index0;
	unsigned int currentSizeTextureIndex = dhIndices.index1;
	unsigned int writeIndex = dhIndices.index2;
	unsigned int mipLevel = dhIndices.index3;

	float2 screenSize = float2(1280 >> mipLevel, 720 >> mipLevel);
	float2 uv = float2(dispatchThreadID.x / screenSize.x, dispatchThreadID.y / screenSize.y);

	float4 downScaledColor = textures[downScaledTextureIndex].SampleLevel(BilinearClamp, uv, 0);
	float4 currentSizeColor = textures[currentSizeTextureIndex][dispatchThreadID.xy];

	float4 finalColor = downScaledColor + currentSizeColor;

	texturesUAV[writeIndex][dispatchThreadID.xy] = finalColor;
	//texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = bloomColor;
	//texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = float4(uv, 0.0f, 1.0f);
}