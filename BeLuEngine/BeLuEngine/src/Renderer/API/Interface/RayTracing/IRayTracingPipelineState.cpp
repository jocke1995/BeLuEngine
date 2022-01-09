#include "stdafx.h"
#include "IRayTracingPipelineState.h"

#include "../IGraphicsManager.h"

#include "../../D3D12/DXR/D3D12RayTracingPipelineState.h"

IRayTracingPipelineState::~IRayTracingPipelineState()
{
}

IRayTracingPipelineState* IRayTracingPipelineState::Create(const RayTracingPSDesc& desc, const std::wstring& name)
{
    E_GRAPHICS_API graphicsApi = IGraphicsManager::GetGraphicsApiType();

    if (graphicsApi == E_GRAPHICS_API::D3D12)
    {
        return new D3D12RayTracingPipelineState(desc, name);
    }
    else if (graphicsApi == E_GRAPHICS_API::VULKAN)
    {
        BL_ASSERT_MESSAGE(false, "Trying to create VulkanRayTracingPipelineState when it is not yet supported!\n");
        return nullptr;
    }
    else
    {
        BL_ASSERT_MESSAGE(false, "Invalid Graphics API");
        return nullptr;
    }
}

RayTracingPSDesc::RayTracingPSDesc()
{
}

RayTracingPSDesc::~RayTracingPSDesc()
{
}
