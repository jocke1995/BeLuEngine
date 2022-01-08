#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float4 worldPos : WPos;
	float2 uv       : UV;
	float3 norm		: NORM;
	float3x3 tbn	: TBN;
};

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex v = meshes[slotInfo.vertexDataIndex][vID];
	float4 vertexPosition = float4(v.pos.xyz, 1.0f);

	output.pos = mul(vertexPosition, matricesPerObject.WVP);
	output.worldPos = mul(vertexPosition, matricesPerObject.worldMatrix);

	output.uv = float2(v.uv.x, v.uv.y);
	
	// Create TBN-Matrix
	float3 T = normalize(mul(float4(v.tang, 0.0f), matricesPerObject.worldMatrix)).xyz;
	float3 N = normalize(mul(float4(v.norm, 0.0f), matricesPerObject.worldMatrix)).xyz;

	// Gram schmidt
	T = normalize(T - dot(T, N) * N);

	float3 B = cross(T, N);

	output.norm = N;
	output.tbn = float3x3(T, B, N);

	return output;
}