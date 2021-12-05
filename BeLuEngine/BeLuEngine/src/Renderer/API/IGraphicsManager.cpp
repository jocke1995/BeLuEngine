#include "stdafx.h"
#include "IGraphicsManager.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "D3D12/D3D12GraphicsManager.h"
TODO("Include future Vulkan Manager");

IGraphicsManager* IGraphicsManager::GetBaseInstance()
{
    BL_ASSERT_MESSAGE(m_sInstance, "IGraphicsManager::m_sInstance is nullptr, did you forget to create it?");

	return m_sInstance;
}

IGraphicsManager::~IGraphicsManager()
{
}

IGraphicsManager* IGraphicsManager::Create(const E_GRAPHICS_API graphicsApi)
{
    BL_ASSERT_MESSAGE(m_sInstance == nullptr, "Trying to create a new graphics manager without deleting the old one!");

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        m_sInstance = new D3D12GraphicsManager();
        return m_sInstance;
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Vulkan not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}

void IGraphicsManager::Destroy()
{
    BL_SAFE_DELETE(m_sInstance);
    m_sInstance = nullptr;
}
