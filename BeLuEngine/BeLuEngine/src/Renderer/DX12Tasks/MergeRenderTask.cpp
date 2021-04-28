#include "stdafx.h"
#include "MergeRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../SwapChain.h"

// Model info
#include "../Geometry/Mesh.h"

MergeRenderTask::MergeRenderTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	m_NumIndices = 0;
	m_Info = {};
}

MergeRenderTask::~MergeRenderTask()
{
}

void MergeRenderTask::AddSRVToMerge(const ShaderResourceView* srvToMerge)
{
	m_SRVs.push_back(srvToMerge);
}

void MergeRenderTask::SetFullScreenQuad(Mesh* mesh)
{
	m_pFullScreenQuadMesh = mesh;
}

void MergeRenderTask::CreateSlotInfo()
{
	// Mesh
	m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_Info.vertexDataIndex = m_pFullScreenQuadMesh->m_pSRV->GetDescriptorHeapIndex();

	// Textures
	// The descriptorHeapIndices for the SRVs are currently put inside the textureSlots inside SlotInfo
	m_dhIndices.index0 = m_SRVs[0]->GetDescriptorHeapIndex();	// Blurred srv
	m_dhIndices.index1 = m_SRVs[1]->GetDescriptorHeapIndex();	// Main color buffer
}

void MergeRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	// Get renderTarget
	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(m_BackBufferIndex);
	ID3D12Resource1* swapChainResource = swapChainRenderTarget->GetResource()->GetID3D12Resource1();

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];

	const unsigned int SwapChainIndex = swapChainRenderTarget->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(SwapChainIndex);

	commandList->SetGraphicsRootSignature(m_pRootSig);

	commandList->SetGraphicsRootDescriptorTable(1, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	// Change state on front/backbuffer
	swapChainRenderTarget->GetResource()->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Change state on mainColorBuffer
	m_SRVs[1]->GetResource()->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandList->OMSetRenderTargets(1, &cdh, true, nullptr);

	const D3D12_VIEWPORT* viewPort = swapChainRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = swapChainRenderTarget->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

	// Draw a fullscreen quad 
	commandList->SetGraphicsRoot32BitConstants(4, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_dhIndices, 0);
	commandList->SetGraphicsRoot32BitConstants(3, sizeof(SlotInfo) / sizeof(UINT), &m_Info, 0);

	commandList->IASetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBufferView());

	commandList->DrawIndexedInstanced(m_NumIndices, 1, 0, 0, 0);

	// Change state on bloomBuffer for next frame
	m_SRVs[0]->GetResource()->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Change state on mainColorBuffer
	m_SRVs[1]->GetResource()->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Change state on front/backbuffer
	swapChainRenderTarget->GetResource()->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	commandList->Close();
}
