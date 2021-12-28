#include "stdafx.h"
#include "GraphicsPass.h"

// API Generic
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsBuffer.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

// API Specific
#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"

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

    for (IGraphicsPipelineState* pState : m_PipelineStates)
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
