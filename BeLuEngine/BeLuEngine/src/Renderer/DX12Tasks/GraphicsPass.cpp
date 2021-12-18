#include "stdafx.h"
#include "GraphicsPass.h"

// DX12 Specifics
#include "../DescriptorHeap.h"

#include "../PipelineState/GraphicsState.h"
#include "../PipelineState/ComputeState.h"

// API Generic
#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsBuffer.h"
#include "../API/IGraphicsTexture.h"

// API Specific
#include "../API/D3D12/D3D12GraphicsContext.h"

GraphicsPass::GraphicsPass(const std::wstring& passName)
	:MultiThreadedTask(F_THREAD_FLAGS::GRAPHICS)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        m_pGraphicsContext = new D3D12GraphicsContext(passName);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        m_pGraphicsContext = nullptr;
        BL_ASSERT_MESSAGE(false, "Trying to create GraphicsPasses with Vulkan when it is not yet supported!\n");
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
    }
}

GraphicsPass::~GraphicsPass()
{
	BL_SAFE_DELETE(m_pGraphicsContext);

    for (PipelineState* pState : m_PipelineStates)
    {
        BL_SAFE_DELETE(pState);
    }
}

void GraphicsPass::AddGraphicsBuffer(std::string id, IGraphicsBuffer* graphicBuffer)
{
	BL_ASSERT_MESSAGE(m_GraphicBuffers[id] == nullptr, "Trying to add graphicBuffer with name: \'%s\' that already exists.\n", id);
	m_GraphicBuffers[id] = graphicBuffer;
}

void GraphicsPass::AddGraphicsTexture(std::string id, IGraphicsTexture* graphicTexture)
{
	BL_ASSERT_MESSAGE(m_GraphicTextures[id] == nullptr, "Trying to add graphicTexture with name: \'%s\' that already exists.\n, id");
	m_GraphicTextures[id] = graphicTexture;
}

IGraphicsContext* const GraphicsPass::GetGraphicsContext() const
{
	BL_ASSERT(m_pGraphicsContext);
	return m_pGraphicsContext;
}
