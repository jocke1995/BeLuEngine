#include "stdafx.h"
#include "IBottomLevelAS.h"

#include "IGraphicsManager.h"

#include "../API/D3D12/DXR/D3D12BottomLevelAS.h"

IBottomLevelAS::~IBottomLevelAS()
{

}

IBottomLevelAS* IBottomLevelAS::Create()
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12BottomLevelAS();
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanBottomLevelAS when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}
