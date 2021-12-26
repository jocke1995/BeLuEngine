#include "stdafx.h"
#include "DepthRenderTask.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Transform.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

DepthRenderTask::DepthRenderTask()
	: GraphicsPass(L"DepthPrePass")
{
	PSODesc psoDesc = {};
	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ALL, BL_COMPARISON_FUNC_LESS);

	psoDesc.AddShader(L"DepthVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"DepthPixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"DepthPrePass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

DepthRenderTask::~DepthRenderTask()
{
}

void DepthRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DepthPrePass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->ClearDepthTexture(m_GraphicTextures["mainDepthStencilBuffer"], true, 1.0f, true, 0);
		m_pGraphicsContext->SetRenderTargets(0, nullptr, m_GraphicTextures["mainDepthStencilBuffer"]);

		// Draw for every Rendercomponent
		for (int i = 0; i < m_RenderComponents.size(); i++)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draws all entities with ModelComponent + TransformComponent
			drawRenderComponent(mc, tc, m_pGraphicsContext);
		}
	}
	m_pGraphicsContext->End();
}

void DepthRenderTask::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

void DepthRenderTask::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, IGraphicsContext* graphicsContext)
{
	// Draw for every m_pMesh the meshComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		const SlotInfo* slotInfo = mc->GetSlotInfoAt(i);

		Transform* t = tc->GetTransform();

		graphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, slotInfo, 0, false);
		graphicsContext->SetConstantBuffer(RootParam_CBV_B2, t->m_pConstantBuffer, false);

		graphicsContext->SetIndexBuffer(m->GetIndexBuffer(), m->GetSizeOfIndices());
		graphicsContext->DrawIndexedInstanced(m->GetNumIndices(), 1, 0, 0, 0);
	}
}
