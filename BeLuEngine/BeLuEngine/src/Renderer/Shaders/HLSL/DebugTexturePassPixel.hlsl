#include "DescriptorBindings.hlsl"
#include "Common.hlsl"

struct VS_OUT
{
	float4 pos      : SV_Position;
	float2 uv       : UV;
};

struct PS_OUTPUT
{
	float4 sceneColor: SV_TARGET0;
};

PS_OUTPUT PS_main(VS_OUT input)
{
	float2 uv = float2(input.uv.x, input.uv.y);

	PS_OUTPUT output = (PS_OUTPUT)0;
	float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);

	// Pick path depending on which texture to visulize
#if defined(ALBEDO)
	finalColor.rgb = textures[cbPerScene.gBufferAlbedo].Sample(Anisotropic16_Wrap, uv).rgb;
#elif defined(NORMAL)
	finalColor.rgb = textures[cbPerScene.gBufferNormal].Sample(Anisotropic16_Wrap, uv).rgb;
#elif defined(ROUGHNESS)
	finalColor.r = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uv).r;
#elif defined(METALLIC)
	finalColor.g = textures[cbPerScene.gBufferMaterialProperties].Sample(Anisotropic16_Wrap, uv).g;
#elif defined(EMISSIVE)
	finalColor.rgb = textures[cbPerScene.gBufferEmissive].Sample(Anisotropic16_Wrap, uv).rgb;
#elif defined(REFLECTION)
	finalColor.rgb = textures[cbPerScene.reflectionTextureSRV].Sample(Anisotropic16_Wrap, uv).rgb;
#elif defined(DEPTH)
	float depth = linearizeDepth(cbPerFrame.nearPlane, cbPerFrame.farPlane, textures[cbPerScene.depth].Sample(Anisotropic16_Wrap, uv).r);
	depth /= cbPerFrame.farPlane;
	finalColor.rgb = float3(depth, depth, depth);
#endif

	output.sceneColor = finalColor;
	return output;
}
