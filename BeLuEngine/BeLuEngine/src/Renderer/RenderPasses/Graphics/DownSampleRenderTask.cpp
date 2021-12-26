#include "stdafx.h"
#include "DownSampleRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsBuffer.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

DownSampleRenderTask::DownSampleRenderTask(IGraphicsTexture* sourceTexture, IGraphicsTexture* destinationTexture, Mesh* fullscreenQuad)
	:GraphicsPass(L"TempDownSampleRenderPass")
{
	PSODesc psoDesc = {};
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.AddShader(L"DownSampleVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"DownSamplePixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"DownSamplePass");
	m_PipelineStates.push_back(iGraphicsPSO);

	m_pSourceTexture = sourceTexture;
	m_pDestinationTexture = destinationTexture;
	m_pFullScreenQuadMesh = fullscreenQuad;
}

DownSampleRenderTask::~DownSampleRenderTask()
{
}

void DownSampleRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DownSamplePass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->SetRenderTargets(1, &m_pDestinationTexture, nullptr);

		SlotInfo slotInfo = {};
		slotInfo.vertexDataIndex = m_pFullScreenQuadMesh->GetVertexBuffer()->GetShaderResourceHeapIndex();

		DescriptorHeapIndices dhIndices = {};
		dhIndices.index0 = m_pSourceTexture->GetShaderResourceHeapIndex();

		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, false);

		m_pGraphicsContext->ResourceBarrier(m_pSourceTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Draw a fullscreen quad
		m_pGraphicsContext->SetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBuffer(), m_pFullScreenQuadMesh->GetSizeOfIndices());
		m_pGraphicsContext->DrawIndexedInstanced(m_pFullScreenQuadMesh->GetNumIndices(), 1, 0, 0, 0);

		m_pGraphicsContext->ResourceBarrier(m_pSourceTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();
}
