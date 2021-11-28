#include "stdafx.h"
#include "DepthRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../SwapChain.h"

// Model info
#include "../Geometry/Mesh.h"
#include "../Geometry/Transform.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"
// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
DepthRenderTask::DepthRenderTask(ID3D12Device5* device, 
	ID3D12RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	: RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
}

DepthRenderTask::~DepthRenderTask()
{
}

void DepthRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(DepthPrePass, commandList);

		commandList->SetGraphicsRootSignature(m_pRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* depthBufferHeap = dsvHeap;

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// TODO: Get Depth viewport, rightnow use swapchain since the view and rect is the same.
		const D3D12_VIEWPORT* viewPort = m_pSwapChain->GetRTV(0)->GetRenderView()->GetViewPort();
		const D3D12_RECT* rect = m_pSwapChain->GetRTV(0)->GetRenderView()->GetScissorRect();
		commandList->RSSetViewports(1, viewPort);
		commandList->RSSetScissorRects(1, rect);

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

		unsigned int index = m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex();

		// Clear and set depth + stencil
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(index);
		commandList->ClearDepthStencilView(dsh, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(0, nullptr, false, &dsh);

		// Draw for every Rendercomponent
		for (int i = 0; i < m_RenderComponents.size(); i++)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draws all entities with ModelComponent + TransformComponent
			drawRenderComponent(mc, tc, viewProjMatTrans, commandList);
		}
	}
	commandList->Close();
}

void DepthRenderTask::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, const DirectX::XMMATRIX* viewProjTransposed, ID3D12GraphicsCommandList5* cl)
{
	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int num_Indices = m->GetNumIndices();
		const SlotInfo* info = mc->GetSlotInfoAt(i);

		Transform* t = tc->GetTransform();
		cl->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0);
		cl->SetGraphicsRootConstantBufferView(RootParam_CBV_B2, t->m_pCB->GetDefaultResource()->GetGPUVirtualAdress());

		cl->IASetIndexBuffer(m->GetIndexBufferView());
		cl->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}
}
