#include "stdafx.h"
#include "DownSampleRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"

TODO("Abstract this")
#include "../Renderer/PipelineState/GraphicsState.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsBuffer.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"

DownSampleRenderTask::DownSampleRenderTask(IGraphicsTexture* sourceTexture, IGraphicsTexture* destinationTexture, Mesh* fullscreenQuad)
	:GraphicsPass(L"TempDownSampleRenderPass")
{
	m_pSourceTexture = sourceTexture;
	m_pDestinationTexture = destinationTexture;
	m_pFullScreenQuadMesh = fullscreenQuad;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDownSampleTexture = {};
	gpsdDownSampleTexture.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdDownSampleTexture.NumRenderTargets = 1;
	gpsdDownSampleTexture.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// Depthstencil usage
	gpsdDownSampleTexture.SampleDesc.Count = 1;
	gpsdDownSampleTexture.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDownSampleTexture.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDownSampleTexture.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDownSampleTexture.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDownSampleTexture.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	gpsdDownSampleTexture.DepthStencilState = dsd;

	m_PipelineStates.push_back(new GraphicsState(L"DownSampleVertex.hlsl", L"DownSamplePixel.hlsl", &gpsdDownSampleTexture, L"DeferredLightPass"));
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

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());
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
