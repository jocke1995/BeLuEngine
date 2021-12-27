#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 256;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int readIndex = dhIndices.index0;
	unsigned int writeIndex = dhIndices.index1;

	float4 inputTexture = textures[readIndex][dispatchThreadID.xy];
	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float brightness = dot(inputTexture.rgb, float3(0.2126, 0.7152, 0.0722));
	if (brightness > 0.5f)
		finalColor = float4(inputTexture.rgb, 1.0);
	else
		finalColor = float4(0.0, 0.0, 0.0, 1.0);

	texturesUAV[writeIndex][dispatchThreadID.xy] = finalColor;
}