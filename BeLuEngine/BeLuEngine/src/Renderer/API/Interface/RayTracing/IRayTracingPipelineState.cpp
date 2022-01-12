#include "stdafx.h"
#include "IRayTracingPipelineState.h"

#include "../Misc/AssetLoader.h"
#include "../Renderer/Shaders/Shader.h"

#include "../IGraphicsManager.h"

#include "../../D3D12/DXR/D3D12RayTracingPipelineState.h"

IRayTracingPipelineState::~IRayTracingPipelineState()
{
}

IRayTracingPipelineState* IRayTracingPipelineState::Create(const RayTracingPSDesc& desc, const std::wstring& name)
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

RayTracingPSDesc::RayTracingPSDesc()
{
    // Set defaults
    m_PayloadSize = 4 * sizeof(unsigned int);
    m_MaxAttributeSize = 2 * sizeof(float);
    m_MaxRecursionDepth = 2;
}

RayTracingPSDesc::~RayTracingPSDesc()
{
}

bool RayTracingPSDesc::AddShader(std::wstring shaderFileName, const std::vector<std::wstring>& shaderEntryPointNames)
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

bool RayTracingPSDesc::AddHitgroup(const std::wstring hitGroupName, const std::wstring closestHitName, const std::wstring anyHitName /*= L"" */, const std::wstring intersectionName /*= L"" */)
{
    m_HitGroups.emplace_back(RayTracingHitGroup(hitGroupName, closestHitName, anyHitName, intersectionName));
    return true;
}

bool RayTracingPSDesc::AddRootSignatureAssociation(const std::vector<IRayTracingRootSignatureParams>& rootSigParams, std::wstring entryPointNameOrHitGroupName, std::wstring rootSigDebugName)
{
    m_RootSignatureAssociations.emplace_back(RayTracingRootSignatureAssociation(rootSigParams, entryPointNameOrHitGroupName, rootSigDebugName));
    return false;
}

void RayTracingPSDesc::SetPayloadSize(unsigned int payloadSize)
{
    m_PayloadSize = payloadSize;
}

void RayTracingPSDesc::SetMaxAttributesSize(unsigned int maxAttributesSize)
{
    m_MaxAttributeSize = maxAttributesSize;
}

void RayTracingPSDesc::SetMaxRecursionDepth(unsigned int maxRecursionDepth)
{
    m_MaxRecursionDepth = maxRecursionDepth;
}

RayTracingShader::RayTracingShader(Shader* shader, const std::vector<std::wstring>& shaderEntryPointNames)
{
    m_Shader = shader;

    m_ShaderEntryPointNames.resize(shaderEntryPointNames.size());
    for (std::wstring shaderEntryPointName : shaderEntryPointNames)
        m_ShaderEntryPointNames.push_back(shaderEntryPointName);
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
    m_RootSigParams.resize(rootSigParams.size());
    for (IRayTracingRootSignatureParams rootSigParam : rootSigParams)
        m_RootSigParams.push_back(rootSigParam);

    m_EntryPointNameOrHitGroupName = entryPointNameOrHitGroupName;
    
    m_RootSigDebugName = rootSigDebugName;
}
