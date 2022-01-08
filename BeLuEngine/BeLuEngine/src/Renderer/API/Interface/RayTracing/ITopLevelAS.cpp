#include "stdafx.h"
#include "ITopLevelAS.h"

#include "../IGraphicsManager.h"

#include "../../D3D12/DXR/D3D12TopLevelAS.h"

ITopLevelAS::~ITopLevelAS()
{

}

ITopLevelAS* ITopLevelAS::Create()
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12TopLevelAS();
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanTopLevelAS when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}
