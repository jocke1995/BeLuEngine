#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
};

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex mesh = meshes[slotInfo.vertexDataIndex][vID];
	float4 vertexPosition = float4(mesh.pos.xyz, 1.0f);

	output.pos = mul(vertexPosition, matricesPerObject.WVP);

	return output;
}