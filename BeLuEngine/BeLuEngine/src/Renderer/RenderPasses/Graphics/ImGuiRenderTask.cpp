#include "stdafx.h"
#include "ImGUIRenderTask.h"

//TEMP
#include "../Renderer/DescriptorHeap.h"

TODO("Fix");
// Generic API
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"
#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"

ImGuiRenderTask::ImGuiRenderTask()
	:GraphicsPass(L"ImGuiPass")
{
}

ImGuiRenderTask::~ImGuiRenderTask()
{
}

void ImGuiRenderTask::Execute()
{
	D3D12GraphicsManager* manager = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance());
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(ImGuiPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		D3D12_RESOURCE_BARRIER bar = {};
		bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		bar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		bar.Transition.pResource = manager->m_SwapchainResources[manager->m_CommandInterfaceIndex];
		bar.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		bar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList->ResourceBarrier(1, &bar);

		DescriptorHeap* renderTargetHeap = manager->GetRTVDescriptorHeap();
		const unsigned int swapChainRTVIndex = manager->m_SwapchainRTVIndices[manager->m_CommandInterfaceIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(swapChainRTVIndex);

		static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList->OMSetRenderTargets(1, &cdh, true, nullptr);

		m_pGraphicsContext->DrawImGui();

		bar = {};
		bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		bar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		bar.Transition.pResource = manager->m_SwapchainResources[manager->m_CommandInterfaceIndex];
		bar.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList->ResourceBarrier(1, &bar);
	}
	m_pGraphicsContext->End();
}
