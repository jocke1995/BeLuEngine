#include "stdafx.h"
#include "ImGUIRenderTask.h"

#include "../CommandInterface.h"
#include "../DescriptorHeap.h"

//ImGui
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"

ImGuiRenderTask::ImGuiRenderTask()
	:GraphicsPass(L"ImGuiPass")
{
}

ImGuiRenderTask::~ImGuiRenderTask()
{
}

void ImGuiRenderTask::Execute()
{
	// Record commandlist
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	ID3D12Resource* swapChainResource = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_SwapchainResources[m_CommandInterfaceIndex];

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(ImGuiPass, commandList);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		// Change state on front/backbuffer
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChainResource,
			D3D12_RESOURCE_STATE_PRESENT,			// StateBefore
			D3D12_RESOURCE_STATE_RENDER_TARGET);	// StateAfter

		commandList->ResourceBarrier(1, &transition);
		DescriptorHeap* renderTargetHeap = rtvHeap;

		const unsigned int swapChainRTVIndex = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_SwapchainRTVIndices[m_CommandInterfaceIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(swapChainRTVIndex);

		commandList->OMSetRenderTargets(1, &cdh, true, nullptr);

		// temp
		//bool a = true;
		//ImGui::ShowDemoWindow(&a);

		ImGui::Render();
		ImDrawData* drawData = ImGui::GetDrawData();
		ImGui_ImplDX12_RenderDrawData(drawData, commandList);

		// Change state on front/backbuffer
		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			swapChainResource,
			D3D12_RESOURCE_STATE_RENDER_TARGET,	// StateBefore
			D3D12_RESOURCE_STATE_PRESENT);		// StateAfter
		commandList->ResourceBarrier(1, &transition);
	}
	commandList->Close();
}
