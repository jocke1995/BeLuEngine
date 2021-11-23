#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float2 uv       : UV;
};

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex mesh = meshes[slotInfo.vertexDataIndex][vID];
	output.pos = float4(mesh.pos.xyz, 1.0f);
	output.uv = mesh.uv;

	return output;
}