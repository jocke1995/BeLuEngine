#include "stdafx.h"
#include "DeferredGeometryRenderTask.h"

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
#include "../Geometry/Material.h"

DeferredGeometryRenderTask::DeferredGeometryRenderTask(ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	: RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
}

DeferredGeometryRenderTask::~DeferredGeometryRenderTask()
{
}

void DeferredGeometryRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(GBufferPass, commandList);

		commandList->SetGraphicsRootSignature(m_pRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(0, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetGraphicsRootDescriptorTable(1, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];
		DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::DSV];

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// TODO: Get Depth viewport, rightnow use swapchain since the view and rect is the same.
		const D3D12_VIEWPORT* viewPort = m_pSwapChain->GetRTV(0)->GetRenderView()->GetViewPort();
		const D3D12_RECT* rect = m_pSwapChain->GetRTV(0)->GetRenderView()->GetScissorRect();
		commandList->RSSetViewports(1, viewPort);
		commandList->RSSetScissorRects(1, rect);

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

		// RenderTargets
		const unsigned int gBufferAlbedoIndex = m_RenderTargetViews["gBufferAlbedo"]->GetDescriptorHeapIndex();
		const unsigned int gBufferNormalIndex = m_RenderTargetViews["gBufferNormal"]->GetDescriptorHeapIndex();
		const unsigned int gBufferMaterialIndex = m_RenderTargetViews["gBufferMaterialProperties"]->GetDescriptorHeapIndex();
		const unsigned int gBufferEmissiveIndex = m_RenderTargetViews["gBufferEmissive"]->GetDescriptorHeapIndex();

		D3D12_CPU_DESCRIPTOR_HANDLE cdhgBufferAlbedo = renderTargetHeap->GetCPUHeapAt(gBufferAlbedoIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhgBufferNormal = renderTargetHeap->GetCPUHeapAt(gBufferNormalIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhgBufferMaterial = renderTargetHeap->GetCPUHeapAt(gBufferMaterialIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhgBufferEmissive = renderTargetHeap->GetCPUHeapAt(gBufferEmissiveIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhgBufferAlbedo, cdhgBufferNormal, cdhgBufferMaterial, cdhgBufferEmissive };

		// Depth
		unsigned int depthIndex = m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(depthIndex);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(cdhgBufferAlbedo, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferNormal, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferMaterial, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferEmissive, clearColor, 0, nullptr);

		commandList->OMSetRenderTargets(4, cdhs, false, &dsh);

		commandList->SetGraphicsRootConstantBufferView(13, m_Resources["cbPerScene"]->GetGPUVirtualAdress());

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

void DeferredGeometryRenderTask::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, const DirectX::XMMATRIX* viewProjTransposed, ID3D12GraphicsCommandList5* cl)
{
	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int num_Indices = m->GetNumIndices();
		const SlotInfo* info = mc->GetSlotInfoAt(i);

		Transform* t = tc->GetTransform();
		cl->SetGraphicsRootConstantBufferView(14, mc->GetMaterialAt(i)->GetMaterialData()->first->GetDefaultResource()->GetGPUVirtualAdress());
		cl->SetGraphicsRoot32BitConstants(3, sizeof(SlotInfo) / sizeof(UINT), info, 0);
		cl->SetGraphicsRootConstantBufferView(11, t->m_pCB->GetDefaultResource()->GetGPUVirtualAdress());

		cl->IASetIndexBuffer(m->GetIndexBufferView());
		cl->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}
}
