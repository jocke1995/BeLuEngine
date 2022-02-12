#include "stdafx.h"
#include "WireframeRenderTask.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"

WireframeRenderTask::WireframeRenderTask()
	:GraphicsPass(L"BoundingBoxPass")
{
	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.SetWireframe();
	psoDesc.SetCullMode(BL_CULL_MODE_NONE);

	psoDesc.AddShader(L"WhiteVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"WhitePixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"BoundingBoxRenderPass");
	m_PipelineStates.push_back(iGraphicsPSO);
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

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, nullptr);

		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);

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
