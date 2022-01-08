#include "stdafx.h"
#include "IGraphicsTexture.h"

#include "IGraphicsManager.h"

#include "../../API/D3D12/D3D12GraphicsTexture.h"

IGraphicsTexture::~IGraphicsTexture()
{
}

IGraphicsTexture* IGraphicsTexture::Create()
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12GraphicsTexture();
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanGraphicsTexture when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}
