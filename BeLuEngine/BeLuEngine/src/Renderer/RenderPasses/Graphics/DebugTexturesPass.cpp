#include "stdafx.h"
#include "DebugTexturesPass.h"

// Shader
#include "../Renderer/Shaders/DXILShaderCompiler.h"

// Model info
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Transform.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"

DebugTexturesPass::DebugTexturesPass()
	: GraphicsPass(L"DebugTexturesPass")
{
	PSODesc psoDesc = {};
	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ALL, BL_COMPARISON_FUNC_LESS);

	{
		DXILCompilationDesc shaderDesc = {};
		shaderDesc.filePath = L"DebugTexturePassVertex.hlsl";
		shaderDesc.shaderType = E_SHADER_TYPE::VS;
		psoDesc.AddShader(shaderDesc);
	}
	{
		DXILCompilationDesc shaderDesc = {};
		shaderDesc.filePath = L"DebugTexturePassPixel.hlsl";
		shaderDesc.shaderType = E_SHADER_TYPE::PS;
		psoDesc.AddShader(shaderDesc);
	}

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"DebugTexturesPass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

DebugTexturesPass::~DebugTexturesPass()
{
}

void DebugTexturesPass::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DebugTextures, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pGraphicsContext->SetViewPort(m_CommonGraphicsResources->renderWidth, m_CommonGraphicsResources->renderHeight);
		m_pGraphicsContext->SetScizzorRect(m_CommonGraphicsResources->renderWidth, m_CommonGraphicsResources->renderHeight);

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, BL_RESOURCE_STATE_DEPTH_WRITE);
		m_pGraphicsContext->ClearDepthTexture(m_CommonGraphicsResources->mainDepthStencil, true, 1.0f, true, 0);
		m_pGraphicsContext->SetRenderTargets(0, nullptr, m_CommonGraphicsResources->mainDepthStencil);

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

void DebugTexturesPass::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

void DebugTexturesPass::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, IGraphicsContext* graphicsContext)
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
