#include "DescriptorBindings.hlsl"

struct VS_OUT
{
	float4 pos  : SV_Position;
	float3 texCoord : TEXCOORD;
};

VS_OUT VS_main(uint vID : SV_VertexID)
{
	VS_OUT output = (VS_OUT)0;

	vertex mesh = meshes[slotInfo.vertexDataIndex][vID];
	float4 vertexPosition = float4(mesh.pos.xyz, 1.0f);

	//Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
	output.pos = mul(float4(vertexPosition.xyz, 1.0f), matricesPerObject.WVP).xyww;

	// Treat the position as a vector, to use when sampling the texture
	output.texCoord = vertexPosition.xyz;

	return output;
}