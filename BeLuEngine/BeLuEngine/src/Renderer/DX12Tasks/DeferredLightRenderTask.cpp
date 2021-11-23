#include "stdafx.h"
#include "DeferredLightRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/PipelineState.h"
#include "../RenderView.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

DeferredLightRenderTask::DeferredLightRenderTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(device, rootSignature, VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	
}

DeferredLightRenderTask::~DeferredLightRenderTask()
{
}

void DeferredLightRenderTask::SetFullScreenQuad(Mesh* mesh)
{
	m_pFullScreenQuadMesh = mesh;

	//m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_Info.vertexDataIndex = m_pFullScreenQuadMesh->m_pVertexBufferSRV->GetDescriptorHeapIndex();
}

void DeferredLightRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	const RenderTargetView* finalColorRTV = m_RenderTargetViews["finalColorBuffer"];
	ID3D12Resource1* finalColorResource = finalColorRTV->GetResource()->GetID3D12Resource1();

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(LightPass, commandList);

		commandList->SetGraphicsRootSignature(m_pRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV];
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::RTV];
		//DescriptorHeap* depthBufferHeap = m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::DSV];

		// RenderTargets
		const unsigned int finalColorTargetIndex = finalColorRTV->GetDescriptorHeapIndex();
		const unsigned int brightTargetIndex = m_RenderTargetViews["brightTarget"]->GetDescriptorHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE cdhgFinalColorTarget = renderTargetHeap->GetCPUHeapAt(finalColorTargetIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhBrightTarget = renderTargetHeap->GetCPUHeapAt(brightTargetIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhgFinalColorTarget, cdhBrightTarget };

		// Depth
		//D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

		commandList->OMSetRenderTargets(2, cdhs, false, nullptr);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(cdhBrightTarget, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgFinalColorTarget, clearColor, 0, nullptr);

		const D3D12_VIEWPORT viewPortFinalColorTarget = *finalColorRTV->GetRenderView()->GetViewPort();
		const D3D12_VIEWPORT viewPortBrightTarget = *m_RenderTargetViews["brightTarget"]->GetRenderView()->GetViewPort();
		const D3D12_VIEWPORT viewPorts[2] = { viewPortFinalColorTarget, viewPortBrightTarget };

		const D3D12_RECT rectFinalColorTarget = *finalColorRTV->GetRenderView()->GetScissorRect();
		const D3D12_RECT rectBrightTarget = *m_RenderTargetViews["brightTarget"]->GetRenderView()->GetScissorRect();
		const D3D12_RECT rects[2] = { rectFinalColorTarget, rectBrightTarget };

		commandList->RSSetViewports(2, viewPorts);
		commandList->RSSetScissorRects(2, rects);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set cbvs
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B3, m_Resources["cbPerFrame"]->GetGPUVirtualAdress());
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B4, m_Resources["cbPerScene"]->GetGPUVirtualAdress());
		commandList->SetGraphicsRootShaderResourceView(RootParam_SRV_T0, m_Resources["rawBufferLights"]->GetGPUVirtualAdress());

		Resource* mainDSV = const_cast<Resource*>(m_pDepthStencil->GetDefaultResource());
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			mainDSV->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,				// StateBefore
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);// StateAfter
		commandList->ResourceBarrier(1, &transition);

		// This pair for m_RenderComponents will be used for model-outlining in case any model is picked.
		//RenderComponent outlinedModel = {nullptr, nullptr};

		// Draw for every Rendercomponent with stencil testing disabled

		commandList->SetPipelineState(m_PipelineStates[0]->GetPSO());
		//component::ModelComponent* mc = m_RenderComponents.at(i).mc;
		//component::TransformComponent* tc = m_RenderComponents.at(i).tc;
		//
		//// If the model is picked, we dont draw it with default stencil buffer.
		//// Instead we store it and draw it later with a different pso to allow for model-outlining
		//if (mc->IsPickedThisFrame() == true)
		//{
		//	outlinedModel = m_RenderComponents.at(i);
		//	continue;
		//}
		//commandList->OMSetStencilRef(1);

		// Draw a fullscreen quad 
		commandList->SetGraphicsRoot32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), &m_Info, 0);
		commandList->IASetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBufferView());

		commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Draw Rendercomponent with stencil testing enabled
		//if (outlinedModel.mc != nullptr)
		//{
		//	commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		//	commandList->OMSetStencilRef(1);
		//	drawRenderComponent(outlinedModel.mc, outlinedModel.tc, viewProjMatTrans, commandList);
		//}

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			mainDSV->GetID3D12Resource1(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// StateBefore
			D3D12_RESOURCE_STATE_DEPTH_WRITE);				// StateAfter
		commandList->ResourceBarrier(1, &transition);

	}
	commandList->Close();
}
