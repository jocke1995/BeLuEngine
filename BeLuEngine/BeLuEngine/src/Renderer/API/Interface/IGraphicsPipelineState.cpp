#include "stdafx.h"
#include "IGraphicsPipelineState.h"

#include "IGraphicsManager.h"

#include "../API/D3D12/D3D12GraphicsPipelineState.h"

IGraphicsPipelineState::~IGraphicsPipelineState()
{
}

IGraphicsPipelineState* IGraphicsPipelineState::Create(const PSODesc& desc, const std::wstring& name)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12GraphicsPipelineState(desc, name);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanGraphicsPipelineState when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}

PSODesc::PSODesc()
{
    // Init defaults

    // Shaders
    for (unsigned int i = 0; i < E_SHADER_TYPE::NUM_SHADER_TYPES; i++)
    {
        m_Shaders[i] = L"";
    }

    // RenderTargets
    for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        m_RenderTargetFormats[i] = BL_FORMAT_UNKNOWN;

        m_RenderTargetBlendDescs[i].enableBlend = false;
        m_RenderTargetBlendDescs[i].enableLogicOP = false;

        m_RenderTargetBlendDescs[i].srcBlend = BL_BLEND::BL_BLEND_ZERO;
        m_RenderTargetBlendDescs[i].destBlend = BL_BLEND::BL_BLEND_ZERO;
        m_RenderTargetBlendDescs[i].blendOp = BL_BLEND_OP::BL_BLEND_OP_ADD;
        m_RenderTargetBlendDescs[i].srcBlendAlpha = BL_BLEND::BL_BLEND_ZERO;
        m_RenderTargetBlendDescs[i].destBlendAlpha = BL_BLEND::BL_BLEND_ZERO;
        m_RenderTargetBlendDescs[i].blendOpAlpha = BL_BLEND_OP::BL_BLEND_OP_ADD;

        m_RenderTargetBlendDescs[i].logicOp = BL_LOGIC_OP::BL_LOGIC_OP_NOOP;
        m_RenderTargetBlendDescs[i].renderTargetWriteMask = BL_COLOR_WRITE_ENABLE_ALL;
    }

    // Depth (Enable writing by default)
    m_DepthStencilDesc.enableDepth = false;
    m_DepthStencilDesc.depthWriteMask = BL_DEPTH_WRITE_MASK::BL_DEPTH_WRITE_MASK_ALL;
    m_DepthStencilDesc.depthComparisonFunc = BL_COMPARISON_FUNC_LESS_EQUAL;

    // Disable stencil
    m_DepthStencilDesc.enableStencil = false;
}

PSODesc::~PSODesc()
{
}

bool PSODesc::AddShader(const std::wstring shaderName, E_SHADER_TYPE shaderType)
{
    BL_ASSERT(shaderType != E_SHADER_TYPE::UNDEFINED);
    BL_ASSERT_MESSAGE(shaderType != E_SHADER_TYPE::DXR, "Cannot create a DXR pipeline state with this class atm!");

    switch (shaderType)
    {
    case E_SHADER_TYPE::VS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A vertexShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::PS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A PixelShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::DS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A DomainShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::HS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A HullShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::GS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A GeometryShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::CS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A ComputeShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::AS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A AmplificationShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    case E_SHADER_TYPE::MS:
        BL_ASSERT_MESSAGE(m_Shaders[shaderType] == L"", "A MeshShader was already added to this PSO!");
        m_Shaders[shaderType] = shaderName;
        break;
    default:
        return false;
    }
    return true;
}

bool PSODesc::AddRenderTargetFormat(BL_FORMAT format)
{
    if (m_NumRenderTargets >= D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        BL_ASSERT(false);
        return false;
    }

    m_RenderTargetFormats[m_NumRenderTargets] = format;
    m_NumRenderTargets++;

    return true;
}

void PSODesc::SetDepthStencilFormat(BL_FORMAT format)
{
    m_DepthStencilFormat = format;
}

void PSODesc::SetDepthDesc(char depthWriteMask, BL_COMPARISON_FUNC depthComparisonFunc)
{
    m_DepthStencilDesc.enableDepth = true;
    m_DepthStencilDesc.depthWriteMask = depthWriteMask;
    m_DepthStencilDesc.depthComparisonFunc = depthComparisonFunc;
}

void PSODesc::SetStencilDesc(char stencilReadMask, char stencilWriteMask, BL_DEPTH_STENCILOP_DESC frontFace, BL_DEPTH_STENCILOP_DESC backFace)
{
    m_DepthStencilDesc.enableStencil = true;
    m_DepthStencilDesc.stencilReadMask = stencilReadMask;
    m_DepthStencilDesc.stencilWriteMask = stencilWriteMask;
    m_DepthStencilDesc.frontFace = frontFace;
    m_DepthStencilDesc.backFace = backFace;
}

void PSODesc::SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY primTop)
{
    m_PrimTop = primTop;
}

void PSODesc::SetCullMode(BL_CULL_MODE cullMode)
{
    m_CullMode = cullMode;
}

void PSODesc::SetWireframe()
{
    m_FillMode = BL_FILL_MODE_WIREFRAME;
}

void PSODesc::AddRenderTargetBlendDesc(BL_BLEND srcBlend, BL_BLEND destBlend, BL_BLEND_OP blendOP, BL_BLEND srcBlendAlpha, BL_BLEND destBlendAlpha, BL_BLEND_OP blendOPAlpha, char renderTargetWriteMask)
{
    BL_ASSERT(m_NumBlendRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].enableBlend = true;;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].srcBlend = srcBlend;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].destBlend = destBlend;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].blendOp = blendOP;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].srcBlendAlpha = srcBlendAlpha;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].destBlendAlpha = destBlendAlpha;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].blendOpAlpha = blendOPAlpha;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].renderTargetWriteMask = renderTargetWriteMask;

    m_NumBlendRenderTargets++;
}

void PSODesc::AddLogicOPRenderTarget(BL_LOGIC_OP logicOP)
{
    BL_ASSERT(m_NumBlendRenderTargets < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].enableLogicOP = true;
    m_RenderTargetBlendDescs[m_NumBlendRenderTargets].logicOp = logicOP;

    m_NumBlendRenderTargets++;
}
