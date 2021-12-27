#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;


[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int bloomTextureIndex = dhIndices.index0;
	unsigned int sceneTextureIndex = dhIndices.index1;
	unsigned int combinedUAVTextureIndex = dhIndices.index2;
	unsigned int mipLevel = dhIndices.index3;

	float2 screenSize = float2(1280.0f, 720.0f);
	float2 uv = float2(dispatchThreadID.x / screenSize.x, dispatchThreadID.y / screenSize.y);
	float4 bloomColor = textures[bloomTextureIndex].SampleLevel(BilinearClamp, uv, 0);

	float4 sceneColor = textures[sceneTextureIndex][dispatchThreadID.xy];

	float4 finalColor = bloomColor + sceneColor;

	texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = finalColor;
	//texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = bloomColor;
	//texturesUAV[combinedUAVTextureIndex][dispatchThreadID.xy] = float4(uv, 0.0f, 1.0f);
}