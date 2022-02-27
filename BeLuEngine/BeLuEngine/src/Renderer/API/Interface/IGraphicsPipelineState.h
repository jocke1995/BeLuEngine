#ifndef IGRAPHICSPIPELINESTATE_H
#define IGRAPHICSPIPELINESTATE_H

#include "RenderCore.h"

#include "../Renderer/Shaders/DXILShaderCompiler.h"

class PSODesc
{
public:
	PSODesc();
	virtual ~PSODesc();

    bool AddShader(const DXILCompilationDesc& desc);
    bool AddRenderTargetFormat(BL_FORMAT format);

    void SetDepthStencilFormat(BL_FORMAT format);
    void SetDepthDesc(char depthWriteMask, BL_COMPARISON_FUNC depthComparisonFunc); // use BL_DEPTH_WRITE_MASK for the depthWriteMask
    void SetStencilDesc(char stencilReadMask, char stencilWriteMask, BL_DEPTH_STENCILOP_DESC frontFace, BL_DEPTH_STENCILOP_DESC backFace);

    void AddRenderTargetBlendDesc(  BL_BLEND srcBlend, BL_BLEND destBlend,
                                BL_BLEND_OP blendOP,
                                BL_BLEND srcBlendAlpha, BL_BLEND destBlendAlpha,
                                BL_BLEND_OP blendOPAlpha,
                                char renderTargetWriteMask = F_BL_RENDER_TARGET_WRITE_MASK::BL_COLOR_WRITE_ENABLE_ALL); // F_BL_RENDER_TARGET_WRITE_MASK
    void AddLogicOPRenderTarget(BL_LOGIC_OP logicOP);

    void SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY primTop);
    void SetCullMode(BL_CULL_MODE cullMode);
    void SetWireframe();
private:
    friend class D3D12GraphicsPipelineState;

    std::vector<DXILCompilationDesc> m_Shaders = {};

    unsigned int m_NumRenderTargets = 0;
    BL_FORMAT m_RenderTargetFormats[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];

    BL_FORMAT m_DepthStencilFormat = BL_FORMAT_UNKNOWN;
    BL_DEPTH_STENCIL_DESC m_DepthStencilDesc = {};

    unsigned int m_NumBlendRenderTargets = 0;
    BL_RENDER_TARGET_BLEND_DESC m_RenderTargetBlendDescs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];

    BL_PRIMITIVE_TOPOLOGY m_PrimTop = BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    BL_CULL_MODE m_CullMode = BL_CULL_MODE_BACK;
    BL_FILL_MODE m_FillMode = BL_FILL_MODE_SOLID;
};

class IGraphicsPipelineState
{
public:
	virtual ~IGraphicsPipelineState();

	static IGraphicsPipelineState* Create(const PSODesc& desc, const std::wstring& name);

protected:

#ifdef DEBUG
	std::wstring m_DebugName = L"";
#endif
};

#endif