#include "stdafx.h"
#include "BlurComputeTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/ComputeState.h"

// Techniques
#include "../Techniques/PingPongResource.h"

TODO(To be replaced by a D3D12Manager some point in the future (needed to access RootSig));
#include "../Renderer.h"

BlurComputeTask::BlurComputeTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	ShaderResourceView* brightSRV,
	const PingPongResource* Bloom0_RESOURCE,
	const PingPongResource* Bloom1_RESOURCE,
	unsigned int screenWidth, unsigned int screenHeight,
	unsigned int FLAG_THREAD)
	:ComputeTask(device, rootSignature, csNamePSOName, FLAG_THREAD, interfaceType)
{
	m_pSRV = brightSRV;

	m_PingPongResources[0] = Bloom0_RESOURCE;
	m_PingPongResources[1] = Bloom1_RESOURCE;

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

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(BlurCompute, commandList);

		commandList->SetComputeRootSignature(m_pRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);
		
		E_GLOBAL_ROOTSIGNATURE;
		commandList->SetComputeRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetComputeRootDescriptorTable(dtUAV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		// Descriptorheap indices for the textures to blur
		// Horizontal pass
		m_DhIndices.index0 = m_PingPongResources[0]->GetSRV()->GetDescriptorHeapIndex();	// Read
		m_DhIndices.index1 = m_PingPongResources[1]->GetUAV()->GetDescriptorHeapIndex();	// Write
		// Vertical pass
		m_DhIndices.index2 = m_PingPongResources[1]->GetSRV()->GetDescriptorHeapIndex();	// Read
		m_DhIndices.index3 = m_PingPongResources[0]->GetUAV()->GetDescriptorHeapIndex();	// Write

		// Send the indices to gpu
		commandList->SetComputeRoot32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_DhIndices, 0);

		// The resource to read (Resource Barrier)
		const_cast<Resource*>(m_PingPongResources[0]->GetSRV()->GetResource())->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// The resource to write (Resource Barrier)
		const_cast<Resource*>(m_PingPongResources[1]->GetUAV()->GetResource())->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur horizontal
		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

		// The resource to write to (Resource Barrier)
		const_cast<Resource*>(m_PingPongResources[1]->GetSRV()->GetResource())->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// The resource to read (Resource Barrier)
		const_cast<Resource*>(m_PingPongResources[0]->GetUAV()->GetResource())->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur vertical
		commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		commandList->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

		// Resource barrier back to original states
		const_cast<Resource*>(m_PingPongResources[0]->GetSRV()->GetResource())->TransResourceState(
			commandList,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	commandList->Close();
}
