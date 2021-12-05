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

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"

DeferredGeometryRenderTask::DeferredGeometryRenderTask(ID3D12Device5* device,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	: RenderTask(device, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
}

DeferredGeometryRenderTask::~DeferredGeometryRenderTask()
{
}

void DeferredGeometryRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(GBufferPass, commandList);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = rtvHeap;
		DescriptorHeap* depthBufferHeap = dsvHeap;

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

		// TODO: Batch into 1 resourceBarrier
		TransferResourceState(m_RenderTargetViews["gBufferAlbedo"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		TransferResourceState(m_RenderTargetViews["gBufferNormal"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		TransferResourceState(m_RenderTargetViews["gBufferMaterialProperties"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		TransferResourceState(m_RenderTargetViews["gBufferEmissive"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Depth
		unsigned int depthIndex = m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(depthIndex);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(cdhgBufferAlbedo, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferNormal, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferMaterial, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgBufferEmissive, clearColor, 0, nullptr);

		commandList->OMSetRenderTargets(4, cdhs, false, &dsh);

		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B4, m_Resources["cbPerScene"]->GetGPUVirtualAdress());

		// Draw for every Rendercomponent
		for (int i = 0; i < m_RenderComponents.size(); i++)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draws all entities with ModelComponent + TransformComponent
			drawRenderComponent(mc, tc, viewProjMatTrans, commandList);
		}

		// TODO: Batch into 1 resourceBarrier
		TransferResourceState(m_RenderTargetViews["gBufferAlbedo"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransferResourceState(m_RenderTargetViews["gBufferNormal"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransferResourceState(m_RenderTargetViews["gBufferMaterialProperties"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		TransferResourceState(m_RenderTargetViews["gBufferEmissive"]->GetResource()->GetID3D12Resource1(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	commandList->Close();
}

void DeferredGeometryRenderTask::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, const DirectX::XMMATRIX* viewProjTransposed, ID3D12GraphicsCommandList5* cl)
{
	cl->SetGraphicsRootShaderResourceView(RootParam_SRV_T1, mc->GetMaterialByteAdressBuffer()->GetDefaultResource()->GetGPUVirtualAdress());

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
