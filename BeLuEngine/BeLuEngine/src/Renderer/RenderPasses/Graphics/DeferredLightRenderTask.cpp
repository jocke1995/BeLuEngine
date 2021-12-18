#include "stdafx.h"
#include "DeferredLightRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

TODO("Abstract this")
#include "../Renderer/PipelineState/GraphicsState.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsBuffer.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"

DeferredLightRenderTask::DeferredLightRenderTask(Mesh* fullscreenQuad)
	:GraphicsPass(L"LightPass")
{
	m_pFullScreenQuadMesh = fullscreenQuad;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDeferredLightRender = {};
	gpsdDeferredLightRender.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdDeferredLightRender.NumRenderTargets = 2;
	gpsdDeferredLightRender.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdDeferredLightRender.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdDeferredLightRender.SampleDesc.Count = 1;
	gpsdDeferredLightRender.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDeferredLightRender.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDeferredLightRender.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDeferredLightRender.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDeferredLightRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = false;
	gpsdDeferredLightRender.DepthStencilState = dsd;
	gpsdDeferredLightRender.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_PipelineStates.push_back(new GraphicsState(L"DeferredLightVertex.hlsl", L"DeferredLightPixel.hlsl", &gpsdDeferredLightRender, L"DeferredLightPass"));
}

DeferredLightRenderTask::~DeferredLightRenderTask()
{
}

void DeferredLightRenderTask::Execute()
{
	BL_ASSERT(m_GraphicTextures["finalColorBuffer"] && m_GraphicTextures["brightTarget"]);

	IGraphicsTexture* renderTargets[2] =
	{
		m_GraphicTextures["finalColorBuffer"],
		m_GraphicTextures["brightTarget"]
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(LightPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);	

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pGraphicsContext->ClearRenderTarget(renderTargets[0], clearColor);
		m_pGraphicsContext->ClearRenderTarget(renderTargets[1], clearColor);

		m_pGraphicsContext->SetRenderTargets(2, renderTargets, nullptr);

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
