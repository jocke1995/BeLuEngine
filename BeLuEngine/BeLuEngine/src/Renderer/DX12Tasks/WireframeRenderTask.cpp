#include "stdafx.h"
#include "WireframeRenderTask.h"

#include "../RenderView.h"
#include "../RootSignature.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../SwapChain.h"
#include "../GPUMemory/Resource.h"
#include "../PipelineState/PipelineState.h"
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Mesh.h"
#include "../Renderer/Camera/BaseCamera.h"
#include "../GPUMemory/RenderTargetView.h"

WireframeRenderTask::WireframeRenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	
}

WireframeRenderTask::~WireframeRenderTask()
{

}

void WireframeRenderTask::AddObjectToDraw(component::BoundingBoxComponent* bbc)
{
	m_ObjectsToDraw.push_back(bbc);
}

void WireframeRenderTask::Clear()
{
	m_ObjectsToDraw.clear();
}

void WireframeRenderTask::ClearSpecific(component::BoundingBoxComponent* bbc)
{
	unsigned int i = 0;
	for (auto& bbcInTask : m_ObjectsToDraw)
	{
		if (bbcInTask == bbc)
		{
			m_ObjectsToDraw.erase(m_ObjectsToDraw.begin() + i);
			break;
		}
		i++;
	}
}

void WireframeRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(m_BackBufferIndex);
	ID3D12Resource1* swapChainResource = swapChainRenderTarget->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	commandList->SetGraphicsRootSignature(m_pRootSig);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	// Change state on front/backbuffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainResource,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV];

	unsigned int renderTargetIndex = m_pSwapChain->GetRTV(m_BackBufferIndex)->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(renderTargetIndex);

	commandList->OMSetRenderTargets(1, &cdh, false, nullptr);

	const D3D12_VIEWPORT* viewPort = swapChainRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = swapChainRenderTarget->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

	const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	// Draw for every mesh
	for (int i = 0; i < m_ObjectsToDraw.size(); i++)
	{
		for (int j = 0; j < m_ObjectsToDraw[i]->GetNumBoundingBoxes(); j++)
		{
			const Mesh* m = m_ObjectsToDraw[i]->GetMeshAt(j);
			Transform* t = m_ObjectsToDraw[i]->GetTransformAt(j);

			unsigned int num_Indices = m->GetNumIndices();
			const SlotInfo* info = m_ObjectsToDraw[i]->GetSlotInfo(j);

			DirectX::XMMATRIX* WTransposed = t->GetWorldMatrixTransposed();
			DirectX::XMMATRIX WVPTransposed = (*viewProjMatTrans) * (*WTransposed);

			// Create a CB_PER_OBJECT struct
			CB_PER_OBJECT_STRUCT perObject = { *WTransposed, WVPTransposed,  *info };

			commandList->SetGraphicsRoot32BitConstants(RS::CB_PER_OBJECT_CONSTANTS, sizeof(CB_PER_OBJECT_STRUCT) / sizeof(UINT), &perObject, 0);

			commandList->IASetIndexBuffer(m->GetIndexBufferView());
			commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
		}
	}

	// �ndra state p� front/backbuffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	commandList->Close();
}
