#include "../../Headers/GPU_Structs.h"

struct VS_OUT
{
	float4 pos  : SV_Position;
};

struct vertex
{
	float3 pos;
	float2 uv;
	float3 norm;
	float3 tang;
};

ConstantBuffer<SlotInfo> info : register(b1, space0);
ConstantBuffer<MATRICES_PER_OBJECT_STRUCT> matricesPerObject : register(b3, space0);

StructuredBuffer<vertex> meshes[] : register(t0, space1);

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex mesh = meshes[info.vertexDataIndex][vID];
	float4 vertexPosition = float4(mesh.pos.xyz, 1.0f);

	output.pos = mul(vertexPosition, matricesPerObject.WVP);

	return output;
}