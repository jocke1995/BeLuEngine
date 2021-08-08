
// Calculate world pos from DepthBuffer
float3 WorldPosFromDepth(float depth, float2 TexCoord, float4x4 projectionInverse, float4x4 viewInverse)
{
	TexCoord.y = 1.0 - TexCoord.y;
	float4 clipSpacePosition = float4(TexCoord * 2.0 - 1.0, depth, 1.0);
	float4 viewSpacePosition = mul(projectionInverse, clipSpacePosition);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	float4 worldSpacePosition = mul(viewInverse, viewSpacePosition);

	return worldSpacePosition.xyz;
}