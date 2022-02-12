#include "stdafx.h"
#include "TransparentRenderTask.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"

TransparentRenderTask::TransparentRenderTask()
	:GraphicsPass(L"TransparentPass")
{
	// Create Front-and Back-culling states
	{
		PSODesc psoDesc = {};
		// RenderTarget (TODO: Formats are way to big atm)
		psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

		psoDesc.SetCullMode(BL_CULL_MODE_FRONT);

		psoDesc.AddRenderTargetBlendDesc(	BL_BLEND_SRC_ALPHA, BL_BLEND_INV_SRC_ALPHA, BL_BLEND_OP_ADD,	// src, dest, OP
											BL_BLEND_ONE, BL_BLEND_ZERO, BL_BLEND_OP_ADD,					// srcAlpha, destAlpha, OPAlpha
											BL_COLOR_WRITE_ENABLE_ALL);										// writeMask

		psoDesc.AddShader(L"TransparentTextureVertex.hlsl", E_SHADER_TYPE::VS);
		psoDesc.AddShader(L"TransparentTexturePixel.hlsl", E_SHADER_TYPE::PS);

		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"BlendFrontCull");
		m_PipelineStates.push_back(iGraphicsPSO);

		psoDesc.SetCullMode(BL_CULL_MODE_BACK);
		iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"BlendBackCull");
		m_PipelineStates.push_back(iGraphicsPSO);
	}
}

TransparentRenderTask::~TransparentRenderTask()
{

}

void TransparentRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(TransparentPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, m_CommonGraphicsResources->mainDepthStencil);

		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_CommonGraphicsResources->cbPerFrame, false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_CommonGraphicsResources->cbPerScene, false);
		m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T0, m_GraphicBuffers["rawBufferLights"], false);

		// Draw from opposite order from the sorted array
		for (int i = m_RenderComponents.size() - 1; i >= 0; i--)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draw for every mesh the MeshComponent has
			// Set material raw buffer
			m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T1, mc->GetMaterialByteAdressBuffer(), false);		
			for (unsigned int j = 0; j < mc->GetNrOfMeshes(); j++)
			{
				Mesh* m = mc->GetMeshAt(j);
				unsigned int num_Indices = m->GetNumIndices();
				const SlotInfo* info = mc->GetSlotInfoAt(j);

				Transform* t = tc->GetTransform();

				m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, info, 0, false);
				m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B2, t->m_pConstantBuffer, false);

				m_pGraphicsContext->SetIndexBuffer(m->GetIndexBuffer(), m->GetSizeOfIndices());

				// Draw each object twice with different PSO 
				for (int k = 0; k < 2; k++)
				{
					m_pGraphicsContext->SetPipelineState(m_PipelineStates[k]);
					m_pGraphicsContext->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
				}
			}
		}
	}
	m_pGraphicsContext->End();
}

void TransparentRenderTask::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}
