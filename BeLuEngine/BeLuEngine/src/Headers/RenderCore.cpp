#include "stdafx.h"
#include "RenderCore.h"

// PIX Events
#include "WinPixEventRuntime/pix3.h"

#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"

ScopedPIXEvent::ScopedPIXEvent(const char* nameOfTask, IGraphicsContext* graphicsContext)
{
	BL_ASSERT(graphicsContext);
    m_pGraphicsContext = graphicsContext;

    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

	UINT64 col = 0;
    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        PIXBeginEvent(static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList, col, nameOfTask);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create a scopedPixEvent with Vulkan when it is not yet supported!\n");
    }
}

ScopedPIXEvent::~ScopedPIXEvent()
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    UINT64 col = 0;
    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        PIXEndEvent(static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create a scopedPixEvent with Vulkan when it is not yet supported!\n");
    }
}