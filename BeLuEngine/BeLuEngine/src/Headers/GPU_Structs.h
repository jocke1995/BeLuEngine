#ifndef STRUCTS_H
#define STRUCTS_H

// Light defines
#define MAX_DIR_LIGHTS   10
#define MAX_POINT_LIGHTS 20
#define MAX_SPOT_LIGHTS  10

enum F_RT_INSTANCE_MASK
{
	INSTANCE_MASK_ENTIRE_SCENE					= 0x01,
	INSTANCE_MASK_SCENE_MINUS_NOSHADOWOBJECTS	= 0x02
};

enum E_RT_SECTION_INDICES
{
	MISS_SHADER_REFLECTIONS = 0,
	MISS_SHADER_SHADOWS = 1
};

struct vertex
{
	float3 pos;
	float2 uv;
	float3 norm;
	float3 tang;
};

// This struct can be used to send specific indices as a root constant to the GPU.
// Example usage is when the indices for pp-effects are sent to gpu.
struct RootConstantUints
{
	unsigned int index0;
	unsigned int index1;
	unsigned int index2;
	unsigned int index3;
};

struct MaterialData
{
	unsigned int textureAlbedo;
	unsigned int textureRoughness;
	unsigned int textureMetallic;
	unsigned int textureNormal;

	unsigned int textureEmissive;
	unsigned int textureOpacity;
	unsigned int hasEmissiveTexture;
	unsigned int hasRoughnessTexture;

	unsigned int hasMetallicTexture;
	unsigned int hasOpacityTexture;
	unsigned int hasNormalTexture;
	unsigned int hasAlbedoTexture;

	float4 emissiveValue;

	float roughnessValue;
	float metallicValue;
	float opacityValue;
	float pad1;

	float4 albedoValue;
};

// Indicies of where the descriptors are stored in the descriptorHeap
struct SlotInfo
{
	unsigned int vertexDataIndex;
	unsigned int indicesDataIndex;
	unsigned int materialIndex;
	unsigned int pad1;
};

struct MATRICES_PER_OBJECT_STRUCT
{
	float4x4 worldMatrix;
	float4x4 WVP;
};

struct CB_PER_FRAME_STRUCT
{
	float3 camPos;
	float pad0;
	float3 camRight;
	float pad1;
	float3 camUp;
	float pad2;
	float3 camForward;
	float pad3;

	float4x4 view;
	float4x4 projection;
	float4x4 viewI;
	float4x4 projectionI;

	float nearPlane;
	float farPlane;
	float2 pad4;
	// deltaTime ..
	// etc ..
};

//TODO("Think about renaming to like.. CbOnDemand.. cause it's currently submitted whenever stuff in it needs update.. ")
//TODO("This could be like the skybox gets changed.. the width/height gets changed, or the buffers get resized so we need the new indices")
struct CB_PER_SCENE_STRUCT
{
	unsigned int rayTracingBVH;
	unsigned int gBufferAlbedo;
	unsigned int gBufferNormal;
	unsigned int gBufferMaterialProperties;

	unsigned int gBufferEmissive;
	unsigned int reflectionTextureSRV;
	unsigned int reflectionTextureUAV;
	unsigned int depth;

	unsigned int skybox;
	unsigned int pad0;
	unsigned int renderingWidth;
	unsigned int renderingHeight;
};

struct LightHeader
{
	unsigned int numDirectionalLights;
	unsigned int numPointLights;
	unsigned int numSpotLights;
	unsigned int pad;
};

struct BaseLight
{
	float3 color;
	float intensity;

	float castShadow;
	float3 pad2;
};

struct DirectionalLight
{
	float4 direction;
	BaseLight baseLight;

	float4x4 viewProj;

	unsigned int textureShadowMap;	// Index to the shadowMap (srv)
	unsigned int pad1[3];
};

struct PointLight
{
	float4 position;

	BaseLight baseLight;
};

struct SpotLight
{
	float4x4 viewProj;

	float4 position_cutOff;			// position  .x.y.z & cutOff in .w (cutOff = radius)
	float4 direction_outerCutoff;	// direction .x.y.z & outerCutOff in .w
	BaseLight baseLight;

	unsigned int textureShadowMap;	// Index to the shadowMap (srv)
	unsigned int pad1[3];
};

#define DIR_LIGHT_MAXOFFSET MAX_DIR_LIGHTS * sizeof(DirectionalLight)
#define POINT_LIGHT_MAXOFFSET MAX_POINT_LIGHTS * sizeof(PointLight)
#define SPOT_LIGHT_MAXOFFSET  MAX_SPOT_LIGHTS * sizeof(SpotLight)

#endif
