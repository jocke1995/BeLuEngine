#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float2 uv       : UV;
};

struct vertex
{
	float3 pos;
	float2 uv;
	float3 norm;
	float3 tang;
};

ConstantBuffer<SlotInfo> info : register(b1, space0);

StructuredBuffer<vertex> meshes[] : register(t0, space1);

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex mesh = meshes[info.vertexDataIndex][vID];
	output.pos = float4(mesh.pos.xyz, 1.0f);
	output.uv = mesh.uv;

	return output;
}