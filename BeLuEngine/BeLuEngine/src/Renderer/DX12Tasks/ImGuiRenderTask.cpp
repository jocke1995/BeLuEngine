#include "stdafx.h"
#include "ImGUIRenderTask.h"

#include "../CommandInterface.h"
#include "../SwapChain.h"
#include "../GPUMemory/GPUMemory.h"

#include "../RootSignature.h"
#include "../DescriptorHeap.h"
#include "../RenderView.h"

//ImGui
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"


ImGuiRenderTask::ImGuiRenderTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	LPCWSTR VSName, LPCWSTR PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	LPCTSTR psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
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

	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(m_BackBufferIndex);
	Resource* swapChainResource = swapChainRenderTarget->GetResource();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	// Change state on front/backbuffer
	swapChainResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];

	const unsigned int swapChainIndex = swapChainRenderTarget->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(swapChainIndex);

	commandList->OMSetRenderTargets(1, &cdh, true, nullptr);

	// temp
	//bool a = true;
	//ImGui::ShowDemoWindow(&a);

	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplDX12_RenderDrawData(drawData, commandList);

	// Change state on front/backbuffer
	swapChainResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	commandList->Close();
}
