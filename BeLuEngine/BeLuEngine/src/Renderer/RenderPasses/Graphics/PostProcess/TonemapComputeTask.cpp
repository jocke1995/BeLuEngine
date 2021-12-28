#include "stdafx.h"
#include "TonemapComputeTask.h"

TODO("Fix this class (remove descriptorHeap stuff)");

#include "../Renderer/API/D3D12/D3D12DescriptorHeap.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"
#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsContext.h"
#include "../Renderer/API/D3D12/D3D12GraphicsPipelineState.h"

constexpr unsigned int g_ThreadGroups = 64;

TonemapComputeTask::TonemapComputeTask(unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"TonemapPass")
{
	PSODesc psoDesc = {};
	psoDesc.AddShader(L"PostProcess/TonemapCompute.hlsl", E_SHADER_TYPE::CS);
	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Tonemap_PSO");
	m_PipelineStates.push_back(iGraphicsPSO);

	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;
}

TonemapComputeTask::~TonemapComputeTask()
{
}

void TonemapComputeTask::Execute()
{
	D3D12GraphicsManager* manager = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance());

	IGraphicsTexture* finalColorTexture = m_GraphicTextures["finalColorBuffer"];
	BL_ASSERT(finalColorTexture);

	unsigned int threadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(m_ScreenWidth) / g_ThreadGroups));
	unsigned int threadGroupsY = m_ScreenHeight;

	auto TransferResourceStateTemp = [this](ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		ID3D12GraphicsCommandList5* tempCL = static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList;

		tempCL->ResourceBarrier(1, &barrier);
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(Tonemap, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(true);

		DescriptorHeapIndices dhIndices = {};
		dhIndices.index0 = finalColorTexture->GetShaderResourceHeapIndex(0);	// Read Un-tonemapped color
		dhIndices.index1 = finalColorTexture->GetUnorderedAccessIndex(0);		// Write tonemapped color

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
		m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);

		m_pGraphicsContext->UAVBarrier(finalColorTexture);

		m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);

		ID3D12GraphicsCommandList5* tempCL = static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList;

		unsigned int backBufferIndex = manager->m_CommandInterfaceIndex;
		ID3D12Resource* destResource = manager->m_SwapchainResources[backBufferIndex];
		ID3D12Resource* srcResource = static_cast<D3D12GraphicsTexture*>(finalColorTexture)->m_pResource;

		TransferResourceStateTemp(destResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		tempCL->CopyResource(destResource, srcResource);
		TransferResourceStateTemp(destResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

		m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();
}
