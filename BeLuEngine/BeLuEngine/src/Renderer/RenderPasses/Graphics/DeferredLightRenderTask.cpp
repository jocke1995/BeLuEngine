#include "stdafx.h"
#include "DeferredLightRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"

DeferredLightRenderTask::DeferredLightRenderTask(Mesh* fullscreenQuad)
	:GraphicsPass(L"LightPass")
{
	m_pFullScreenQuadMesh = fullscreenQuad;

	PSODesc psoDesc = {};
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);	// sceneColor (HDR-range)

	psoDesc.AddShader(L"DeferredLightVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"DeferredLightPixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"LightPass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

DeferredLightRenderTask::~DeferredLightRenderTask()
{
}

void DeferredLightRenderTask::Execute()
{
	IGraphicsTexture* renderTargets[4] =
	{
		m_CommonGraphicsResources->gBufferAlbedo,
		m_CommonGraphicsResources->gBufferNormal,
		m_CommonGraphicsResources->gBufferMaterialProperties,
		m_CommonGraphicsResources->gBufferEmissive
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(LightPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);	

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);
		
		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, BL_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ClearRenderTarget(m_CommonGraphicsResources->finalColorBuffer, clearColor);

		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, nullptr);

		// Set cbvs
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_CommonGraphicsResources->cbPerFrame, false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_CommonGraphicsResources->cbPerScene, false);
		m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T0, m_GraphicBuffers["rawBufferLights"], false);

		// Set States
		m_pGraphicsContext->ResourceBarrier(renderTargets[0], BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[1], BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[2], BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[3], BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->reflectionTexture, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Draw a fullscreen quad 
		SlotInfo slotInfo = {};
		slotInfo.vertexDataIndex = m_pFullScreenQuadMesh->GetVertexBuffer()->GetShaderResourceHeapIndex();
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);

		m_pGraphicsContext->SetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBuffer(), m_pFullScreenQuadMesh->GetSizeOfIndices());

		m_pGraphicsContext->DrawIndexedInstanced(m_pFullScreenQuadMesh->GetNumIndices(), 1, 0, 0, 0);
	}
	m_pGraphicsContext->End();
}
