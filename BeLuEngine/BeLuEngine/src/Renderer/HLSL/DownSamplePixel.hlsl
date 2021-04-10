#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float2 uv   : UV;
};

// Source descriptorHeapIndex is stored in albedo
ConstantBuffer<DescriptorHeapIndices> dhIndices : register(b2, space3);

Texture2D<float4> textures[]   : register (t0);

SamplerState linear_Wrap	: register (s5);

float4 PS_main(VS_OUT input) : SV_TARGET0
{
	float4 outputFiltered = textures[dhIndices.index0].Sample(linear_Wrap, input.uv);

	return outputFiltered;
}