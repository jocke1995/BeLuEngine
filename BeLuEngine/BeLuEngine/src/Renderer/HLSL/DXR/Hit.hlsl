#include "hlslhelpers.hlsl"
#include "../LightCalculations.hlsl"	// This includes "DescriptorBindings.hlsl"
 
/* LOCAL */ 
ByteAddressBuffer rawBufferSlotInfo: register(t6, space0);
ByteAddressBuffer rawBufferMaterialData : register(t7, space0);

ConstantBuffer<MATRICES_PER_OBJECT_STRUCT> localMatricesPerObject : register(b8, space0);
 
[shader("closesthit")]  
void ClosestHit(inout ReflectionPayload reflectionPayload, in BuiltInTriangleIntersectionAttributes attribs)
{ 
    float3 bary = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y); 
    
    SlotInfo slotInfo = rawBufferSlotInfo.Load<SlotInfo>(GeometryIndex() * sizeof(SlotInfo));
    MaterialData matData = rawBufferMaterialData.Load<MaterialData>(GeometryIndex() * sizeof(MaterialData));
    
    uint vertId = 3 * PrimitiveIndex();
    unsigned int IndexID1 = indices[slotInfo.indicesDataIndex][vertId + 0];
    unsigned int IndexID2 = indices[slotInfo.indicesDataIndex][vertId + 1];
    unsigned int IndexID3 = indices[slotInfo.indicesDataIndex][vertId + 2];
    
    // Get the normal 
    vertex v1 = meshes[slotInfo.vertexDataIndex][IndexID1];
    vertex v2 = meshes[slotInfo.vertexDataIndex][IndexID2]; 
    vertex v3 = meshes[slotInfo.vertexDataIndex][IndexID3]; 
    
	float4 tmp = mul(float4(v1.pos, 1.0f), localMatricesPerObject.worldMatrix);
    float3 norm = v1.norm * bary.x + v2.norm * bary.y + v3.norm * bary.z; 
    float3 normal = float3(normalize(mul(float4(norm, 0.0f), localMatricesPerObject.worldMatrix)).xyz);
	
    float2 uv = v1.uv * bary.x + v2.uv * bary.y + v3.uv * bary.z; 
    
    float4 albedo	= textures[matData.textureAlbedo].SampleLevel(BilinearWrap, uv, 2);
	float roughness	= matData.hasRoughnessTexture ? textures[matData.textureRoughness].SampleLevel(BilinearWrap, uv, 2).r : matData.roughnessValue;
	float metallic	= matData.hasMetallicTexture  ? textures[matData.textureMetallic].SampleLevel(BilinearWrap, uv, 2).g  : matData.metallicValue;
	float4 emissive = matData.hasEmissiveTexture ? textures[matData.textureEmissive].SampleLevel(BilinearWrap, uv, 2) : matData.emissiveValue;

	float3 worldPos = WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
	float3 camPos = cbPerFrame.camPos;
	float3 viewDir = normalize(camPos - worldPos.xyz);
	
	if (roughness < 0.11 && metallic > 0.9f)
	{
		RayDesc ray;
		ray.Origin = float4(worldPos, 1.0f) + float4(normal.xyz, 0.0f) * 0.5f;
		ray.Direction = normalize(reflect(WorldRayDirection(), float3(normal.xyz)));
		ray.TMin = 0;
		ray.TMax = 10000;
	
		// Trace the ray
		reflectionPayload.color += TraceRadianceRay(ray, reflectionPayload.recursionDepth, sceneBVH[cbPerScene.rayTracingBVH]);
	}

	// Do lightning calculations
	float3 baseReflectivity = lerp(float3(0.04f, 0.04f, 0.04f), albedo.rgb, metallic);
	
	LightHeader lHeader = rawBufferLights.Load<LightHeader>(0);
	// DirectionalLight contributions
	for (unsigned int i = 0; i < lHeader.numDirectionalLights; i++)
	{
		DirectionalLight dirLight = rawBufferLights.Load<DirectionalLight>(sizeof(LightHeader) + i * sizeof(DirectionalLight));
	
		reflectionPayload.color += CalcDirLight(
			dirLight,
			camPos,
			viewDir,
			float4(worldPos, 1.0f),
			metallic,
			albedo.rgb,
			roughness,
			normal,
			baseReflectivity);
	}
	
	// PointLight contributions
	for (unsigned int i = 0; i < lHeader.numPointLights; i++)
	{
		PointLight pointLight = rawBufferLights.Load<PointLight>(sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET + i * sizeof(PointLight));

		float3 lightColor = CalcPointLight(
			pointLight,
			camPos,
			viewDir,
			float4(worldPos, 1.0f),
			metallic,
			albedo.rgb,
			roughness,
			normal,
			baseReflectivity);

		float3 lightDir = normalize(pointLight.position.xyz - worldPos.xyz);
		//float shadowFactor = RT_ShadowFactor(worldPos.xyz, 0.1f, length(pointLight.position.xyz - worldPos.xyz) - 1.0, lightDir, sceneBVH[cbPerScene.rayTracingBVH]);

		reflectionPayload.color += lightColor; //* shadowFactor;
	}

	reflectionPayload.color += 0.001f * albedo.rgb;
	reflectionPayload.color += (emissive.rgb * emissive.a);
} 