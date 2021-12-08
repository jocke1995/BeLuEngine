#include "stdafx.h"
#include "DownSampleRenderTask.h"

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
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"

DownSampleRenderTask::DownSampleRenderTask(
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	IGraphicsTexture* sourceTexture,
	IGraphicsTexture* destinationTexture,
	unsigned int FLAG_THREAD)
	:RenderTask(VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	m_pSourceTexture = sourceTexture;
	m_pDestinationTexture = destinationTexture;
	m_NumIndices = 0;
	m_Info = {};
}

DownSampleRenderTask::~DownSampleRenderTask()
{
}

void DownSampleRenderTask::SetFullScreenQuad(Mesh* mesh)
{
	m_pFullScreenQuadMesh = mesh;
}

void DownSampleRenderTask::SetFullScreenQuadInSlotInfo()
{
	// Mesh
	m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_Info.vertexDataIndex = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetVertexBuffer())->GetShaderResourceHeapIndex();

	// The descriptorHeapIndices for the source&dest are currently put inside the textureSlots inside SlotInfo
	m_dhIndices.index0 = static_cast<D3D12GraphicsTexture*>(m_pSourceTexture)->GetShaderResourceHeapIndex();
}

void DownSampleRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(DownSamplePass, commandList);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_RTV = rtvHeap;
		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

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

		unsigned int rtvIndex = static_cast<D3D12GraphicsTexture*>(m_pDestinationTexture)->GetRenderTargetHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = descriptorHeap_RTV->GetCPUHeapAt(rtvIndex);
		commandList->OMSetRenderTargets(1, &cdh, false, nullptr);

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

		commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), &m_Info, 0);
		commandList->SetGraphicsRoot32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / sizeof(UINT), &m_dhIndices, 0);

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = m_pFullScreenQuadMesh->GetSizeOfIndices();
		commandList->IASetIndexBuffer(&indexBufferView);

		ID3D12Resource1* sourceResource = static_cast<D3D12GraphicsTexture*>(m_pSourceTexture)->GetTempResource();
		
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			sourceResource,
			D3D12_RESOURCE_STATE_RENDER_TARGET,				// StateBefore
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter
		commandList->ResourceBarrier(1, &transition);

		// Draw a fullscreen quad
		commandList->DrawIndexedInstanced(m_NumIndices, 1, 0, 0, 0);

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			sourceResource,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// StateBefore
			D3D12_RESOURCE_STATE_RENDER_TARGET);		// StateAfter
		commandList->ResourceBarrier(1, &transition);

	}
	commandList->Close();
}
