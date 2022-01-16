#include "stdafx.h"
#include "IRayTracingPipelineState.h"

#include "../Misc/AssetLoader.h"
#include "../Renderer/Shaders/Shader.h"

#include "../IGraphicsManager.h"

#include "../../D3D12/DXR/D3D12RayTracingPipelineState.h"

IRayTracingPipelineState::~IRayTracingPipelineState()
{
}

IRayTracingPipelineState* IRayTracingPipelineState::Create(const RayTracingPipelineStateDesc& desc, const std::wstring& name)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12RayTracingPipelineState(desc, name);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanRayTracingPipelineState when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}

RayTracingPipelineStateDesc::RayTracingPipelineStateDesc()
{
    // Set defaults
    m_PayloadSize = 4 * sizeof(unsigned int);
    m_MaxAttributeSize = 2 * sizeof(float);
    m_MaxRecursionDepth = 2;
}

RayTracingPipelineStateDesc::~RayTracingPipelineStateDesc()
{
}

bool RayTracingPipelineStateDesc::AddShader(std::wstring shaderFileName, const std::wstring& shaderEntryPointNames)
{
    Shader* shader = AssetLoader::Get()->loadShader(shaderFileName, E_SHADER_TYPE::DXR);

    if (shader == false)
    {
        BL_LOG_CRITICAL("Failed to load Shader: %S", shaderFileName);
        return false;
    }

    m_RayTracingShaders.emplace_back(RayTracingShader(shader, shaderEntryPointNames));
    return true;
}

bool RayTracingPipelineStateDesc::AddHitgroup(const std::wstring hitGroupName, const std::wstring closestHitName, const std::wstring anyHitName /*= L"" */, const std::wstring intersectionName /*= L"" */)
{
    m_HitGroups.emplace_back(RayTracingHitGroup(hitGroupName, closestHitName, anyHitName, intersectionName));
    return true;
}

bool RayTracingPipelineStateDesc::AddRootSignatureAssociation(const std::vector<IRayTracingRootSignatureParams>& rootSigParams, std::wstring entryPointNameOrHitGroupName, std::wstring rootSigDebugName)
{
    m_RootSignatureAssociations.emplace_back(RayTracingRootSignatureAssociation(rootSigParams, entryPointNameOrHitGroupName, rootSigDebugName));
    return false;
}

void RayTracingPipelineStateDesc::SetPayloadSize(unsigned int payloadSize)
{
    m_PayloadSize = payloadSize;
}

void RayTracingPipelineStateDesc::SetMaxAttributesSize(unsigned int maxAttributesSize)
{
    BL_ASSERT(maxAttributesSize < D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES);
    m_MaxAttributeSize = maxAttributesSize;
}

void RayTracingPipelineStateDesc::SetMaxRecursionDepth(unsigned int maxRecursionDepth)
{
    BL_ASSERT(maxRecursionDepth <= D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH);
    m_MaxRecursionDepth = maxRecursionDepth;
}

RayTracingShader::RayTracingShader(Shader* shader, const std::wstring& shaderEntryPointNames)
{
    m_Shader = shader;

    unsigned int numShaderEntryPointNames = shaderEntryPointNames.size();
    m_ShaderEntryPointNames.resize(numShaderEntryPointNames);
    for (unsigned int i = 0; i < numShaderEntryPointNames; i++)
        m_ShaderEntryPointNames[i] = shaderEntryPointNames[i];
}

RayTracingHitGroup::RayTracingHitGroup(const std::wstring hitGroupName, const std::wstring closestHitName, const std::wstring anyHitName, const std::wstring intersectionName)
{
    m_HitGroupName = hitGroupName;
    m_ClosestHitName = closestHitName;
    m_AnyHitName = anyHitName;
    m_IntersectionName = intersectionName;
}

RayTracingRootSignatureAssociation::RayTracingRootSignatureAssociation(const std::vector<IRayTracingRootSignatureParams>& rootSigParams, std::wstring entryPointNameOrHitGroupName, std::wstring rootSigDebugName)
{
    unsigned int numRootSigParams = rootSigParams.size();
    m_RootSigParams.resize(numRootSigParams);
    for (unsigned int i = 0; i < numRootSigParams; i++)
        m_RootSigParams[i] = rootSigParams[i];

    m_EntryPointNameOrHitGroupName = entryPointNameOrHitGroupName;
    
    m_RootSigDebugName = rootSigDebugName;
}
