#include "stdafx.h"
#include "WireframeRenderTask.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"

TODO("Abstract this")
#include "../PipelineState/GraphicsState.h"

// Generic API
#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsBuffer.h"
#include "../API/IGraphicsTexture.h"
#include "../API/IGraphicsContext.h"

WireframeRenderTask::WireframeRenderTask()
	:GraphicsPass(L"BoundingBoxPass")
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdWireFrame = {};
	gpsdWireFrame.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdWireFrame.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdWireFrame.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdWireFrame.SampleDesc.Count = 1;
	gpsdWireFrame.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdWireFrame.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	gpsdWireFrame.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsdWireFrame.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC blendRTdesc{};
	blendRTdesc.BlendEnable = false;
	blendRTdesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendRTdesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendRTdesc.BlendOp = D3D12_BLEND_OP_ADD;
	blendRTdesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendRTdesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendRTdesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendRTdesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// Specify Blend descriptions
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdWireFrame.BlendState.RenderTarget[i] = blendRTdesc;

	m_PipelineStates.push_back(new GraphicsState(L"WhiteVertex.hlsl", L"WhitePixel.hlsl", &gpsdWireFrame, L"BoundingBoxRenderPass"));
}

WireframeRenderTask::~WireframeRenderTask()
{

}

void WireframeRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(WireFramePass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetRenderTargets(1, &m_GraphicTextures["finalColorBuffer"], nullptr);

		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		// Draw for every Component
		for (int i = 0; i < m_ObjectsToDraw.size(); i++)
		{
			// Draw for every mesh
			for (int j = 0; j < m_ObjectsToDraw[i]->GetNumBoundingBoxes(); j++)
			{
				const Mesh* m = m_ObjectsToDraw[i]->GetMeshAt(j);
				Transform* t = m_ObjectsToDraw[i]->GetTransformAt(j);

				unsigned int num_Indices = m->GetNumIndices();
				const SlotInfo* info = m_ObjectsToDraw[i]->GetSlotInfo(j);

				m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / sizeof(UINT), info, 0, false);
				m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B2, t->m_pConstantBuffer, false);

				m_pGraphicsContext->SetIndexBuffer(m->GetIndexBuffer(), m->GetSizeOfIndices());
				m_pGraphicsContext->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
			}
		}
	}
	m_pGraphicsContext->End();
}

void WireframeRenderTask::AddObjectToDraw(component::BoundingBoxComponent* bbc)
{
	m_ObjectsToDraw.push_back(bbc);
}

void WireframeRenderTask::Clear()
{
	m_ObjectsToDraw.clear();
}

void WireframeRenderTask::ClearSpecific(component::BoundingBoxComponent* bbc)
{
	unsigned int i = 0;
	for (auto& bbcInTask : m_ObjectsToDraw)
	{
		if (bbcInTask == bbc)
		{
			m_ObjectsToDraw.erase(m_ObjectsToDraw.begin() + i);
			break;
		}
		i++;
	}
}
