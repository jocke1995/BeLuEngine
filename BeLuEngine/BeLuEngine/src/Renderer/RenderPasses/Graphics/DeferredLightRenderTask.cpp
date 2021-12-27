#include "stdafx.h"
#include "DeferredLightRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsBuffer.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

DeferredLightRenderTask::DeferredLightRenderTask(Mesh* fullscreenQuad)
	:GraphicsPass(L"LightPass")
{
	m_pFullScreenQuadMesh = fullscreenQuad;

	PSODesc psoDesc = {};
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

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
	BL_ASSERT(m_GraphicTextures["finalColorBuffer"]);

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(LightPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);	

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pGraphicsContext->ClearRenderTarget(m_GraphicTextures["finalColorBuffer"], clearColor);

		m_pGraphicsContext->SetRenderTargets(1, &m_GraphicTextures["finalColorBuffer"], nullptr);

		// Set cbvs
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_GraphicBuffers["cbPerFrame"], false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_GraphicBuffers["cbPerScene"], false);
		m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T0, m_GraphicBuffers["rawBufferLights"], false);

		m_pGraphicsContext->ResourceBarrier(m_GraphicTextures["mainDepthStencilBuffer"], D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// Draw a fullscreen quad 
		SlotInfo slotInfo = {};
		slotInfo.vertexDataIndex = m_pFullScreenQuadMesh->GetVertexBuffer()->GetShaderResourceHeapIndex();
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);

		m_pGraphicsContext->SetIndexBuffer(m_pFullScreenQuadMesh->GetIndexBuffer(), m_pFullScreenQuadMesh->GetSizeOfIndices());

		m_pGraphicsContext->DrawIndexedInstanced(m_pFullScreenQuadMesh->GetNumIndices(), 1, 0, 0, 0);

		m_pGraphicsContext->ResourceBarrier(m_GraphicTextures["mainDepthStencilBuffer"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	m_pGraphicsContext->End();
}
