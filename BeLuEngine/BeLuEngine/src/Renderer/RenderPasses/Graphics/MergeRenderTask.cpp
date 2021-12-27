#include "stdafx.h"
#include "MergeRenderTask.h"

TODO("Remove this");
#include "../Renderer/DescriptorHeap.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"
#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"
#include "../Renderer/API/D3D12/D3D12GraphicsPipelineState.h"

MergeRenderTask::MergeRenderTask(Mesh* fullscreenQuad)
	:GraphicsPass(L"MergeRenderPass")
{
	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.AddShader(L"MergeVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"MergePixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"MergeRenderPass");
	m_PipelineStates.push_back(iGraphicsPSO);

	m_pFullScreenQuadMesh = fullscreenQuad;
}

MergeRenderTask::~MergeRenderTask()
{
}

void MergeRenderTask::Execute()
{
	D3D12GraphicsManager* manager = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance());

	auto TransferResourceState = [this](ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList->ResourceBarrier(1, &barrier);
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(MergePass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		DescriptorHeap* renderTargetHeap = manager->GetRTVDescriptorHeap();

		const unsigned int SwapChainRTVIndex = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_SwapchainRTVIndices[manager->m_CommandInterfaceIndex];
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = renderTargetHeap->GetCPUHeapAt(SwapChainRTVIndex);

		// Change state on front/backbuffer
		ID3D12Resource* swapChainResource = D3D12GraphicsManager::GetInstance()->m_SwapchainResources[manager->m_CommandInterfaceIndex];
		TransferResourceState(swapChainResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Change state on mainColorBuffer
		ID3D12Resource* mainColorBufferResource = static_cast<D3D12GraphicsTexture*>(m_GraphicTextures["finalColorBuffer"])->m_pResource;
		TransferResourceState(mainColorBufferResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList->OMSetRenderTargets(1, &cdh, true, nullptr);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		SlotInfo slotInfo = {};
		slotInfo.vertexDataIndex = static_cast<D3D12GraphicsBuffer*>(m_pFullScreenQuadMesh->GetVertexBuffer())->GetShaderResourceHeapIndex();

		// Textures to merge
		DescriptorHeapIndices dhIndices = {};
		dhIndices.index1 = m_GraphicTextures["finalColorBuffer"]->GetShaderResourceHeapIndex();		// Main color buffer
		dhIndices.index2 = m_GraphicTextures["reflectionTexture"]->GetShaderResourceHeapIndex();	// Reflection Data

		// Draw a fullscreen quad 
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, false);

		m_pGraphicsContext->SetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBuffer(), m_pFullScreenQuadMesh->GetSizeOfIndices());

		m_pGraphicsContext->DrawIndexedInstanced(m_pFullScreenQuadMesh->GetNumIndices(), 1, 0, 0, 0);

		// Change state on mainColorBuffer
		TransferResourceState(mainColorBufferResource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Change state on front/backbuffer
		TransferResourceState(swapChainResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	}
	m_pGraphicsContext->End();
}
