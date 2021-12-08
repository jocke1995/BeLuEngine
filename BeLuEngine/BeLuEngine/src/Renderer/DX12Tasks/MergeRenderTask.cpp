#include "stdafx.h"
#include "MergeRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/PipelineState.h"

// Model info
#include "../Geometry/Mesh.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"

MergeRenderTask::MergeRenderTask(
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	m_NumIndices = 0;
	m_Info = {};
}

MergeRenderTask::~MergeRenderTask()
{
}

void MergeRenderTask::SetFullScreenQuad(Mesh* mesh)
{
	m_pFullScreenQuadMesh = mesh;
}

void MergeRenderTask::CreateSlotInfo()
{
	// Mesh
	m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_Info.vertexDataIndex = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetVertexBuffer())->GetShaderResourceHeapIndex();

	// Textures
	// The descriptorHeapIndices for the SRVs are currently put inside the textureSlots inside SlotInfo
	m_dhIndices.index0 = m_GraphicTextures["bloomPingPong0"]->GetShaderResourceHeapIndex();		// Blurred srv
	m_dhIndices.index1 = m_GraphicTextures["finalColorBuffer"]->GetShaderResourceHeapIndex();	// Main color buffer
	m_dhIndices.index2 = m_GraphicTextures["reflectionTexture"]->GetShaderResourceHeapIndex();	// Reflection Data
}

void MergeRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	auto TransferResourceState = [commandList](ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
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
		ScopedPixEvent(MergePass, commandList);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		DescriptorHeap* renderTargetHeap = rtvHeap;

		const unsigned int SwapChainRTVIndex = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_SwapchainRTVIndices[m_CommandInterfaceIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(SwapChainRTVIndex);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		// Change state on front/backbuffer
		ID3D12Resource* swapChainResource = D3D12GraphicsManager::GetInstance()->m_SwapchainResources[m_CommandInterfaceIndex];
		TransferResourceState(swapChainResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Change state on mainColorBuffer
		ID3D12Resource* mainColorBufferResource = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->GetTempResource();
		TransferResourceState(mainColorBufferResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->OMSetRenderTargets(1, &cdh, true, nullptr);

		TODO("Fix the sizes");
		D3D12_VIEWPORT viewPort = {};
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		viewPort.Width = 1280;
		viewPort.Height = 720;
		viewPort.MinDepth = 0.0f;
		viewPort.MaxDepth = 1.0f;

		D3D12_RECT rect = {};
		rect.left = 0;
		rect.right = 1280;
		rect.top = 0;
		rect.bottom = 720;

		commandList->RSSetViewports(1, &viewPort);
		commandList->RSSetScissorRects(1, &rect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

		// Draw a fullscreen quad 
		commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), &m_Info, 0);
		commandList->SetGraphicsRoot32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_dhIndices, 0);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = m_pFullScreenQuadMesh->GetSizeOfIndices();
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(m_NumIndices, 1, 0, 0, 0);

		// Change state on bloomBuffer for next frame
		ID3D12Resource* bloomPingPong0Texture = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["bloomPingPong0"])->GetTempResource();
		TransferResourceState(bloomPingPong0Texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Change state on mainColorBuffer
		TransferResourceState(mainColorBufferResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		//float col[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
		//commandList->ClearRenderTargetView(cdh, col, 0, nullptr);

		// Change state on front/backbuffer
		TransferResourceState(swapChainResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		
	}
	commandList->Close();
}
