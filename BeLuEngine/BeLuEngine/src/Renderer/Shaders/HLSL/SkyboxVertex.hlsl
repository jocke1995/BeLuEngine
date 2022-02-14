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

	float4x4 viewMat = cbPerFrame.view;

	viewMat[0][3] = 0;
	viewMat[1][3] = 0;
	viewMat[2][3] = 0;

	float4x4 projMat = cbPerFrame.projection;
	float4x4 vp = mul(viewMat, projMat);

	//Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
	output.pos = mul(float4(vertexPosition.xyz, 1.0f), vp).xyww;

	// Treat the position as a vector, to use when sampling the texture
	output.texCoord = vertexPosition.xyz;

	return output;
}