#include "stdafx.h"
#include "IGraphicsBuffer.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "IGraphicsManager.h"

#include "../API/D3D12/D3D12GraphicsBuffer.h"

IGraphicsBuffer::~IGraphicsBuffer()
{
}


IGraphicsBuffer* IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE type, unsigned int sizeOfSingleItem, unsigned int numItems, DXGI_FORMAT format, std::wstring name)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12GraphicsBuffer(type, sizeOfSingleItem, numItems, format, name);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanGraphicsBuffer when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}
