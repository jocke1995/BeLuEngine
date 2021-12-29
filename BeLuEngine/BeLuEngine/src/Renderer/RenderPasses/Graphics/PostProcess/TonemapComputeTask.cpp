#include "stdafx.h"
#include "TonemapComputeTask.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

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
	IGraphicsTexture* finalColorTexture = m_GraphicTextures["finalColorBuffer"];
	BL_ASSERT(finalColorTexture);

	unsigned int threadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(m_ScreenWidth) / g_ThreadGroups));
	unsigned int threadGroupsY = m_ScreenHeight;

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(Tonemap, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(true);

		RootConstantUints rootConstantUints = {};
		rootConstantUints.index0 = finalColorTexture->GetShaderResourceHeapIndex(0);	// Read Un-tonemapped color
		rootConstantUints.index1 = finalColorTexture->GetUnorderedAccessIndex(0);		// Write tonemapped color

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(RootConstantUints) / 4, &rootConstantUints, 0, true);
		m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);

		m_pGraphicsContext->UAVBarrier(finalColorTexture);

		m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();
}
