#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	// Source descriptorHeapIndex is stored in albedo
	float4 outputFiltered = textures[dhIndices.index0].Sample(MIN_MAG_MIP_LINEAR_Wrap, input.uv);

	return outputFiltered;
}