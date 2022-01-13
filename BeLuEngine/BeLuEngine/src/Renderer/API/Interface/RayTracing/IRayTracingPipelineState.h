#ifndef IRAYTRACINGPIPELINESTATE_H
#define IRAYTRACINGPIPELINESTATE_H

#include "RenderCore.h"

class Shader;

struct RayTracingShader
{
	RayTracingShader(Shader* shader, const std::wstring& shaderEntryPointNames);

	Shader* m_Shader = nullptr;
    std::wstring m_ShaderEntryPointNames;
};

struct RayTracingHitGroup
{
	RayTracingHitGroup(const std::wstring hitGroupName, const std::wstring closestHitName, const std::wstring anyHitName = L"", const std::wstring intersectionName = L"");
	std::wstring m_HitGroupName = L"";
	std::wstring m_ClosestHitName = L"";
	std::wstring m_AnyHitName = L"";
	std::wstring m_IntersectionName = L"";
};

TODO("Add support for descriptorTables and 32BitConstants");
struct IRayTracingRootSignatureParams
{
	BL_ROOT_PARAMETER_TYPE rootParamType = (BL_ROOT_PARAMETER_TYPE)0;
	unsigned int shaderRegister = 0;
	unsigned int registerSpace = 0;
};

struct RayTracingRootSignatureAssociation
{
	RayTracingRootSignatureAssociation(const std::vector<IRayTracingRootSignatureParams>& rootSigParams, std::wstring entryPointNameOrHitGroupName, std::wstring rootSigDebugName);
	std::vector<IRayTracingRootSignatureParams> m_RootSigParams;
	std::wstring m_EntryPointNameOrHitGroupName;
	std::wstring m_RootSigDebugName;
};

class RayTracingPSDesc
{
public:
	RayTracingPSDesc();
	virtual ~RayTracingPSDesc();

	// Example: AddShader(L"myRayGenerationTestShader.hlsl", L"RayGen");
	bool AddShader(std::wstring shaderFileName, const std::wstring& shaderEntryPointNames);

	// AnyHit and Intersection are optional
	bool AddHitgroup(const std::wstring hitGroupName, const std::wstring closestHitName, const std::wstring anyHitName = L"", const std::wstring intersectionName = L"");

	// Example as name: L"ReflectionHitGroup", or L"RayGen" as in the entry point. rootSigDebugName is optional
	bool AddRootSignatureAssociation(const std::vector<IRayTracingRootSignatureParams>& rootSigParams, std::wstring entryPointNameOrHitGroupName, std::wstring rootSigDebugName = L"dxrRootSig");

	void SetPayloadSize(unsigned int payloadSize);
	void SetMaxAttributesSize(unsigned int maxAttributesSize);
	void SetMaxRecursionDepth(unsigned int maxRecursionDepth);
private:
    friend class D3D12RayTracingPipelineState;

    std::vector<RayTracingShader> m_RayTracingShaders;
    std::vector<RayTracingHitGroup> m_HitGroups;
    std::vector<RayTracingRootSignatureAssociation> m_RootSignatureAssociations;

	unsigned int m_PayloadSize = 0;
	unsigned int m_MaxAttributeSize = 0;
	unsigned int m_MaxRecursionDepth = 0;
};

class IRayTracingPipelineState
{
public:
	virtual ~IRayTracingPipelineState();

	static IRayTracingPipelineState* Create(const RayTracingPSDesc& desc, const std::wstring& name);

protected:

#ifdef DEBUG
	std::wstring m_DebugName = L"";
#endif
};

#endif