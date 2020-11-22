#include "stdafx.h"
#include "OutliningRenderTask.h"

#include "../RenderView.h"
#include "../RootSignature.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/Resource.h"
#include "../GraphicsState.h"
#include "../SwapChain.h"
#include "../Renderer/Transform.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Camera/BaseCamera.h"
#include "../GPUMemory/RenderTargetView.h"
#include "../GPUMemory/DepthStencil.h"
#include "../GPUMemory/DepthStencilView.h"


OutliningRenderTask::OutliningRenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	// Init with nullptr
	Clear();
}

OutliningRenderTask::~OutliningRenderTask()
{
}

void OutliningRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);
	const RenderTargetView* swapChainRenderTarget = m_pSwapChain->GetRTV(m_BackBufferIndex);
	ID3D12Resource1* swapChainResource = swapChainRenderTarget->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV];
	DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(m_BackBufferIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

	// Check if there is an object to outline
	if (m_ObjectToOutline.first == nullptr)
	{
		commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
		commandList->Close();
		return;
	}
	// else continue as usual

	commandList->SetGraphicsRootSignature(m_pRootSig);
	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	// Change state on front/backbuffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainResource,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET));

	commandList->OMSetRenderTargets(1, &cdh, true, &dsh);

	const D3D12_VIEWPORT* viewPort = swapChainRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_RECT* rect = swapChainRenderTarget->GetRenderView()->GetScissorRect();
	commandList->RSSetViewports(1, viewPort);
	commandList->RSSetScissorRects(1, rect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());

	const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	// Draw for every m_pMesh
	for (int i = 0; i < m_ObjectToOutline.first->GetNrOfMeshes(); i++)
	{
		Mesh* m = m_ObjectToOutline.first->GetMeshAt(i);
		Transform* t = m_ObjectToOutline.second->GetTransform();
		Transform newScaledTransform = *t;
		newScaledTransform.IncreaseScaleByPercent(0.02f);
		newScaledTransform.UpdateWorldMatrix();

		component::ModelComponent* mc = m_ObjectToOutline.first;

		size_t num_Indices = m->GetNumIndices();
		const SlotInfo* info = mc->GetSlotInfoAt(i);

		DirectX::XMMATRIX* WTransposed = newScaledTransform.GetWorldMatrixTransposed();
		DirectX::XMMATRIX WVPTransposed = (*viewProjMatTrans) * (*WTransposed);

		// Create a CB_PER_OBJECT struct
		CB_PER_OBJECT_STRUCT perObject = { *WTransposed, WVPTransposed, *info };

		commandList->SetGraphicsRoot32BitConstants(RS::CB_PER_OBJECT_CONSTANTS, sizeof(CB_PER_OBJECT_STRUCT) / sizeof(UINT), &perObject, 0);

		commandList->IASetIndexBuffer(m->GetIndexBufferView());

		commandList->OMSetStencilRef(1);
		commandList->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}

	// Change state on front/backbuffer
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		swapChainResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT));

	commandList->Close();
}

void OutliningRenderTask::SetObjectToOutline(std::pair<component::ModelComponent*, component::TransformComponent*>* objectToOutline)
{
	m_ObjectToOutline = *objectToOutline;
}

void OutliningRenderTask::Clear()
{
	m_ObjectToOutline.first = nullptr;
	m_ObjectToOutline.second = nullptr;
}
