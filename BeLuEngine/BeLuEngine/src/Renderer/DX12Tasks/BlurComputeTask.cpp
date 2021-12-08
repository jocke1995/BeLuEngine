#include "stdafx.h"
#include "BlurComputeTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/ComputeState.h"

TODO(To be replaced by a D3D12Manager some point in the future (needed to access RootSig));
#include "../Renderer.h"

#include "../Techniques/Bloom.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"

BlurComputeTask::BlurComputeTask(
	std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	Bloom* bloom,
	unsigned int screenWidth, unsigned int screenHeight,
	unsigned int FLAG_THREAD)
	:ComputeTask(csNamePSOName, FLAG_THREAD, interfaceType)
{
	m_pTempBloom = bloom;

	m_HorizontalThreadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(screenWidth) / m_ThreadsPerGroup));
	m_HorizontalThreadGroupsY = screenHeight;

	m_VerticalThreadGroupsX = screenWidth;
	m_VerticalThreadGroupsY = static_cast<unsigned int>(ceilf(static_cast<float>(screenHeight) / m_ThreadsPerGroup));
}

BlurComputeTask::~BlurComputeTask()
{

}

void BlurComputeTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();

	auto TransferResourceState = [commandList](IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		ID3D12Resource* resource = static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempResource();
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		commandList->ResourceBarrier(1, &barrier);
	};

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(BlurCompute, commandList);

		commandList->SetComputeRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);
		
		E_GLOBAL_ROOTSIGNATURE;
		commandList->SetComputeRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetComputeRootDescriptorTable(dtUAV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		// Descriptorheap indices for the textures to blur
		// Horizontal pass
		m_DhIndices.index0 = m_pTempBloom->GetPingPongTexture(0)->GetShaderResourceHeapIndex();	// Read
		m_DhIndices.index1 = m_pTempBloom->GetPingPongTexture(1)->GetUnorderedAccessIndex();	// Write
		// Vertical pass
		m_DhIndices.index2 = m_pTempBloom->GetPingPongTexture(1)->GetShaderResourceHeapIndex();	// Read
		m_DhIndices.index3 = m_pTempBloom->GetPingPongTexture(0)->GetUnorderedAccessIndex();	// Write

		// Send the indices to gpu
		commandList->SetComputeRoot32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_DhIndices, 0);

		// The resource to read (Resource Barrier)
		TransferResourceState(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// The resource to write (Resource Barrier)
		TransferResourceState(m_pTempBloom->GetPingPongTexture(1), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur horizontal
		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

		// The resource to write to (Resource Barrier)
		TransferResourceState(m_pTempBloom->GetPingPongTexture(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// The resource to read (Resource Barrier)
		TransferResourceState(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur vertical
		commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		commandList->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

		// Resource barrier back to original states
		TransferResourceState(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	commandList->Close();
}
