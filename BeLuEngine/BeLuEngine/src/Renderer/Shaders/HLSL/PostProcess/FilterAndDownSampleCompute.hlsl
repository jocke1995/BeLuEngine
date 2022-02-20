#include "../DescriptorBindings.hlsl"

static const int g_NumThreads = 64;

[numthreads(g_NumThreads, 1, 1)]
void CS_main(uint3 dispatchThreadID : SV_DispatchThreadID, int3 groupThreadID : SV_GroupThreadID)
{
	unsigned int readIndex = rootConstantUints.index0;
	unsigned int writeIndex = rootConstantUints.index1;

	float2 screenSize = float2(cbPerScene.renderingWidth, cbPerScene.renderingHeight) / 2;
	float2 uv = float2(dispatchThreadID.x / screenSize.x, dispatchThreadID.y / screenSize.y);
	float4 sceneColor = textures[readIndex].SampleLevel(BilinearClamp, uv, 0);

	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	float brightness = dot(sceneColor.rgb, float3(0.2126, 0.7152, 0.0722));
	if (brightness > 1.0f)
		finalColor = float4(sceneColor.rgb, 1.0);
	else
		finalColor = float4(0.0, 0.0, 0.0, 1.0);

	texturesUAV[writeIndex][dispatchThreadID.xy] = finalColor;
	//texturesUAV[writeIndex][dispatchThreadID.xy] = float4(uv, 0.0f, 1.0f);
}