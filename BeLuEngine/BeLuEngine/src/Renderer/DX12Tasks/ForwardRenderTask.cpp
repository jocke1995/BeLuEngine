#include "stdafx.h"
#include "ForwardRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"
#include "../RootSignature.h"

// Model info
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Mesh.h"

ForwardRenderTask::ForwardRenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	
}

ForwardRenderTask::~ForwardRenderTask()
{
}

void ForwardRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	const RenderTargetView* mainColorRenderTarget = m_RenderTargetViews["mainColorTarget"];
	ID3D12Resource1* mainColorTargetResource = mainColorRenderTarget->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	commandList->SetGraphicsRootSignature(m_pRootSig);
	
	DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
	commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	commandList->SetGraphicsRootDescriptorTable(RS::dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
	commandList->SetGraphicsRootDescriptorTable(RS::dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

	DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV];
	DescriptorHeap* depthBufferHeap  = m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV];

	// RenderTargets
	const unsigned int mainColorTargetIndex = mainColorRenderTarget->GetDescriptorHeapIndex();
	const unsigned int brightTargetIndex = m_RenderTargetViews["brightTarget"]->GetDescriptorHeapIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE cdhMainColorTarget = renderTargetHeap->GetCPUHeapAt(mainColorTargetIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE cdhBrightTarget = renderTargetHeap->GetCPUHeapAt(brightTargetIndex);
	D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhMainColorTarget, cdhBrightTarget };

	// Depth
	D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

	commandList->OMSetRenderTargets(2, cdhs, false, &dsh);

	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(cdhMainColorTarget, clearColor, 0, nullptr);
	commandList->ClearRenderTargetView(cdhBrightTarget, clearColor, 0, nullptr);

	const D3D12_VIEWPORT viewPortMainColorTarget = *mainColorRenderTarget->GetRenderView()->GetViewPort();
	const D3D12_VIEWPORT viewPortBrightTarget = *m_RenderTargetViews["brightTarget"]->GetRenderView()->GetViewPort();
	const D3D12_VIEWPORT viewPorts[2] = { viewPortMainColorTarget, viewPortBrightTarget };

	const D3D12_RECT rectMainColorTarget = *mainColorRenderTarget->GetRenderView()->GetScissorRect();
	const D3D12_RECT rectBrightTarget = *m_RenderTargetViews["brightTarget"]->GetRenderView()->GetScissorRect();
	const D3D12_RECT rects[2] = { rectMainColorTarget, rectBrightTarget };

	commandList->RSSetViewports(2, viewPorts);
	commandList->RSSetScissorRects(2, rects);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set cbvs
	commandList->SetGraphicsRootConstantBufferView(RS::CB_PER_FRAME, m_Resources["cbPerFrame"]->GetGPUVirtualAdress());
	commandList->SetGraphicsRootConstantBufferView(RS::CB_PER_SCENE, m_Resources["cbPerScene"]->GetGPUVirtualAdress());

	const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();

	// This pair for m_RenderComponents will be used for model-outlining in case any model is picked.
	std::pair<component::ModelComponent*, component::TransformComponent*> outlinedModel = std::make_pair(nullptr, nullptr);

	// Draw for every Rendercomponent with stencil testing disabled
	commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
	for (int i = 0; i < m_RenderComponents.size(); i++)
	{
		
		component::ModelComponent* mc = m_RenderComponents.at(i).first;
		component::TransformComponent* tc = m_RenderComponents.at(i).second;

		// If the model is picked, we dont draw it with default stencil buffer.
		// Instead we store it and draw it later with a different pso to allow for model-outlining
		if (mc->IsPickedThisFrame() == true)
		{
			outlinedModel = std::make_pair(m_RenderComponents.at(i).first, m_RenderComponents.at(i).second);
			continue;
		}
		commandList->OMSetStencilRef(1);
		drawRenderComponent(mc, tc, viewProjMatTrans, commandList);
	}

	// Draw Rendercomponent with stencil testing enabled
	if (outlinedModel.first != nullptr)
	{
		commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		commandList->OMSetStencilRef(1);
		drawRenderComponent(outlinedModel.first, outlinedModel.second, viewProjMatTrans, commandList);
	}

	commandList->Close();
}

void ForwardRenderTask::drawRenderComponent(
	component::ModelComponent* mc,
	component::TransformComponent* tc,
	const DirectX::XMMATRIX* viewProjTransposed,
	ID3D12GraphicsCommandList5* cl)
{
	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int num_Indices = m->GetNumIndices();
		const SlotInfo* info = mc->GetSlotInfoAt(i);

		Transform* transform = tc->GetTransform();
		DirectX::XMMATRIX* WTransposed = transform->GetWorldMatrixTransposed();
		DirectX::XMMATRIX WVPTransposed = (*viewProjTransposed) * (*WTransposed);

		// Create a CB_PER_OBJECT struct
		CB_PER_OBJECT_STRUCT perObject = { *WTransposed, WVPTransposed, *info };

		cl->SetGraphicsRoot32BitConstants(RS::CB_PER_OBJECT_CONSTANTS, sizeof(CB_PER_OBJECT_STRUCT) / sizeof(UINT), &perObject, 0);

		cl->IASetIndexBuffer(m->GetIndexBufferView());
		cl->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
	}
}
