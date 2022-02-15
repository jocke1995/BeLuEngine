#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float3 texCoord : TEXCOORD;
};

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	unsigned int skyboxDescriptorIndex = cbPerScene.skybox;

	return cubeTextures[skyboxDescriptorIndex].Sample(BilinearWrap, input.texCoord);
}