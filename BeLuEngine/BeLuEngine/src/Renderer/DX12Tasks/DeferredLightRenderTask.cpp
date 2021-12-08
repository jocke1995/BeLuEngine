#include "stdafx.h"
#include "DeferredLightRenderTask.h"

// DX12 Specifics
#include "../Camera/BaseCamera.h"
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../PipelineState/PipelineState.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"

TODO(To be replaced by a D3D12Manager some point in the future(needed to access RootSig));
#include "../Renderer.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"


DeferredLightRenderTask::DeferredLightRenderTask(
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:RenderTask(VSName, PSName, gpsds, psoName, FLAG_THREAD)
{
	
}

DeferredLightRenderTask::~DeferredLightRenderTask()
{
}

void DeferredLightRenderTask::SetFullScreenQuad(Mesh* mesh)
{
	m_pFullScreenQuadMesh = mesh;

	//m_NumIndices = m_pFullScreenQuadMesh->GetNumIndices();
	m_Info.vertexDataIndex = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetVertexBuffer())->GetShaderResourceHeapIndex();
}

void DeferredLightRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();

	D3D12GraphicsTexture* finalColorTexture = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"]);
	D3D12GraphicsTexture* brightColorTexture = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["brightTarget"]);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(LightPass, commandList);

		commandList->SetGraphicsRootSignature(static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig);

		DescriptorHeap* descriptorHeap_CBV_UAV_SRV = mainHeap;
		ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap_CBV_UAV_SRV->GetID3D12DescriptorHeap();
		commandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

		commandList->SetGraphicsRootDescriptorTable(dtCBV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));
		commandList->SetGraphicsRootDescriptorTable(dtSRV, descriptorHeap_CBV_UAV_SRV->GetGPUHeapAt(0));

		DescriptorHeap* renderTargetHeap = rtvHeap;

		// RenderTargets
		const unsigned int finalColorTargetIndex = finalColorTexture->GetRenderTargetHeapIndex();
		const unsigned int brightTargetIndex = brightColorTexture->GetRenderTargetHeapIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE cdhgFinalColorTarget = renderTargetHeap->GetCPUHeapAt(finalColorTargetIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhBrightTarget = renderTargetHeap->GetCPUHeapAt(brightTargetIndex);
		D3D12_CPU_DESCRIPTOR_HANDLE cdhs[] = { cdhgFinalColorTarget, cdhBrightTarget };

		// Depth
		//D3D12_CPU_DESCRIPTOR_HANDLE dsh = depthBufferHeap->GetCPUHeapAt(m_pDepthStencil->GetDSV()->GetDescriptorHeapIndex());

		commandList->OMSetRenderTargets(2, cdhs, false, nullptr);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(cdhBrightTarget, clearColor, 0, nullptr);
		commandList->ClearRenderTargetView(cdhgFinalColorTarget, clearColor, 0, nullptr);

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


		const D3D12_VIEWPORT viewPorts[2] = { viewPort, viewPort };
		const D3D12_RECT rects[2] = { rect, rect };

		commandList->RSSetViewports(2, viewPorts);
		commandList->RSSetScissorRects(2, rects);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set cbvs
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B3, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["cbPerFrame"])->GetTempResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(RootParam_CBV_B4, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["cbPerScene"])->GetTempResource()->GetGPUVirtualAddress());
		commandList->SetGraphicsRootShaderResourceView(RootParam_SRV_T0, static_cast<D3D12GraphicsBuffer*>(m_GraphicBuffers["rawBufferLights"])->GetTempResource()->GetGPUVirtualAddress());

		D3D12GraphicsTexture* mainDSV = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["mainDepthStencilBuffer"]);
		CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
			mainDSV->GetTempResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,				// StateBefore
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);	// StateAfter
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

		D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
		indexBufferView.BufferLocation = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetIndexBuffer())->GetTempResource()->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = m_pFullScreenQuadMesh->GetSizeOfIndices();
		commandList->IASetIndexBuffer(&indexBufferView);

		commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		// Draw Rendercomponent with stencil testing enabled
		//if (outlinedModel.mc != nullptr)
		//{
		//	commandList->SetPipelineState(m_PipelineStates[1]->GetPSO());
		//	commandList->OMSetStencilRef(1);
		//	drawRenderComponent(outlinedModel.mc, outlinedModel.tc, viewProjMatTrans, commandList);
		//}

		transition = CD3DX12_RESOURCE_BARRIER::Transition(
			mainDSV->GetTempResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,	// StateBefore
			D3D12_RESOURCE_STATE_DEPTH_WRITE);				// StateAfter
		commandList->ResourceBarrier(1, &transition);

	}
	commandList->Close();
}
