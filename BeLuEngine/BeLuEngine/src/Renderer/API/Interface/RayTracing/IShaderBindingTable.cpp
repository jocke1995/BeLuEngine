#include "stdafx.h"
#include "IRayTracingPipelineState.h"

#include "../IGraphicsManager.h"

#include "../../D3D12/DXR/D3D12ShaderBindingTable.h"
#include "IShaderBindingTable.h"


IShaderBindingTable::~IShaderBindingTable()
{
}

IShaderBindingTable* IShaderBindingTable::Create(const std::wstring& name)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12ShaderBindingTable(name);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanShaderBindingTable when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}

void IShaderBindingTable::Reset()
{
    m_RayGenerationRecords.clear();
    m_MissRecords.clear();
    m_HitGroupRecords.clear();
}

void IShaderBindingTable::AddShaderRecord(E_SHADER_RECORD_TYPE shaderRecordType, const std::wstring& entryPoint, const std::vector<IGraphicsBuffer*>& inputData)
{
    switch (shaderRecordType)
    {
        case E_SHADER_RECORD_TYPE::RAY_GENERATION_SHADER_RECORD:
        {
            m_RayGenerationRecords.emplace_back(entryPoint, inputData);
            break;
        }
        case E_SHADER_RECORD_TYPE::MISS_SHADER_RECORD:
        {
            m_MissRecords.emplace_back(entryPoint, inputData);
            break;
        }
        case E_SHADER_RECORD_TYPE::HIT_GROUP_SHADER_RECORD:
        {
            m_HitGroupRecords.emplace_back(entryPoint, inputData);
            break;
        }
        default:
        {
            BL_ASSERT_MESSAGE(false, "Inaccurate usage of API!\n");
            break;
        }
    }
}

unsigned int IShaderBindingTable::GetRayGenSectionSize()
{
    return mMaxRayGenerationSize * static_cast<unsigned int>(m_RayGenerationRecords.size());
}

unsigned int IShaderBindingTable::GetMissSectionSize()
{
    return mMaxMissRecordSize * static_cast<unsigned int>(m_MissRecords.size());
}

unsigned int IShaderBindingTable::GetHitGroupSectionSize()
{
    return mMaxHitGroupSize * static_cast<unsigned int>(m_HitGroupRecords.size());
}

unsigned int IShaderBindingTable::GetMaxRayGenShaderRecordSize()
{
    BL_ASSERT(mMaxRayGenerationSize);
    return mMaxRayGenerationSize;
}

unsigned int IShaderBindingTable::GetMaxMissShaderRecordSize()
{
    BL_ASSERT(mMaxMissRecordSize);
    return mMaxMissRecordSize;
}

unsigned int IShaderBindingTable::GetMaxHitGroupShaderRecordSize()
{
    BL_ASSERT(mMaxHitGroupSize);
    return mMaxHitGroupSize;
}

ShaderRecord::ShaderRecord(const std::wstring entryPointName, const std::vector<IGraphicsBuffer*> inputData)
{
    m_EntryPointName = entryPointName;

    unsigned int numInputDatas = inputData.size();
    if (numInputDatas > 0)
    {
        m_InputData.resize(numInputDatas);
        for (unsigned int i = 0; i < numInputDatas; i++)
        {
            m_InputData[i] = inputData[i];
        }
    }
}
