#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int downScaledTextureIndex = rootConstantUints.index0;
	unsigned int currentSizeTextureIndex = rootConstantUints.index1;
	unsigned int writeIndex = rootConstantUints.index2;
	unsigned int mipLevel = rootConstantUints.index3;

	float2 textureSize = float2(1280 >> mipLevel, 720 >> mipLevel);
	// TODO: the "+ 1.0f"-part is kinda hacky...
	float2 uv = float2((dispatchThreadID.x + 1.0f) / textureSize.x, (dispatchThreadID.y + 1.0) / textureSize.y);
	//float2 uv = float2(dispatchThreadID.x / screenSize.x, dispatchThreadID.y / screenSize.y);

	float4 downScaledColor = textures[downScaledTextureIndex].SampleLevel(BilinearClamp, uv, 0);
	float4 currentSizeColor = textures[currentSizeTextureIndex][dispatchThreadID.xy];

	float4 finalColor = downScaledColor + currentSizeColor;

	texturesUAV[writeIndex][dispatchThreadID.xy] = finalColor;
}