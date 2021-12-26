#include "stdafx.h"
#include "DeferredGeometryRenderTask.h"

// Model info
#include "../../Geometry/Mesh.h"
#include "../../Geometry/Transform.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../../API/IGraphicsManager.h"
#include "../../API/IGraphicsBuffer.h"
#include "../../API/IGraphicsTexture.h"
#include "../../API/IGraphicsContext.h"
#include "../../API/IGraphicsPipelineState.h"

DeferredGeometryRenderTask::DeferredGeometryRenderTask()
	: GraphicsPass(L"GeometryPass")
{
	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ZERO, BL_COMPARISON_FUNC_LESS_EQUAL);

	//psoDesc.SetWireframe();

	psoDesc.AddShader(L"DeferredGeometryVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"DeferredGeometryPixel.hlsl", E_SHADER_TYPE::PS);
	
	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"GeometryPass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

DeferredGeometryRenderTask::~DeferredGeometryRenderTask()
{
}

void DeferredGeometryRenderTask::Execute()
{
	BL_ASSERT(	m_GraphicTextures["gBufferAlbedo"] && 
				m_GraphicTextures["gBufferNormal"] && 
				m_GraphicTextures["gBufferMaterialProperties"] && 
				m_GraphicTextures["gBufferEmissive"]);

	IGraphicsTexture* renderTargets[4] =
	{
		m_GraphicTextures["gBufferAlbedo"],
		m_GraphicTextures["gBufferNormal"],
		m_GraphicTextures["gBufferMaterialProperties"],
		m_GraphicTextures["gBufferEmissive"]
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(GBufferPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		TODO("Batch into 1 resourceBarrier");
		m_pGraphicsContext->ResourceBarrier(renderTargets[0], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(renderTargets[1], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(renderTargets[2], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(renderTargets[3], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

		float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_pGraphicsContext->ClearRenderTarget(renderTargets[0], clearColor);
		m_pGraphicsContext->ClearRenderTarget(renderTargets[1], clearColor);
		m_pGraphicsContext->ClearRenderTarget(renderTargets[2], clearColor);
		m_pGraphicsContext->ClearRenderTarget(renderTargets[3], clearColor);
		
		m_pGraphicsContext->SetRenderTargets(4, renderTargets, m_GraphicTextures["mainDepthStencilBuffer"]);

		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_GraphicBuffers["cbPerScene"], false);

		// Draw for every Rendercomponent
		for (int i = 0; i < m_RenderComponents.size(); i++)
		{
			component::ModelComponent* mc = m_RenderComponents.at(i).mc;
			component::TransformComponent* tc = m_RenderComponents.at(i).tc;

			// Draws all entities
			drawRenderComponent(mc, tc, m_pGraphicsContext);
		}

		// TODO: Batch into 1 resourceBarrier
		m_pGraphicsContext->ResourceBarrier(renderTargets[0], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[1], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[2], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_pGraphicsContext->ResourceBarrier(renderTargets[3], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	m_pGraphicsContext->End();
}

void DeferredGeometryRenderTask::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

void DeferredGeometryRenderTask::drawRenderComponent(component::ModelComponent* mc, component::TransformComponent* tc, IGraphicsContext* graphicsContext)
{
	// Rawbytebuffer of the material for this model
	m_pGraphicsContext->SetShaderResourceView(RootParam_SRV_T1, mc->GetMaterialByteAdressBuffer(), false);

	// Draw for every mesh the modelComponent has
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Mesh* m = mc->GetMeshAt(i);
		unsigned int numIndices = m->GetNumIndices();
		unsigned int sizeOfIndices = m->GetSizeOfIndices();
		const SlotInfo* slotInfo = mc->GetSlotInfoAt(i);

		Transform* t = tc->GetTransform();
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, slotInfo, 0, false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B2, t->m_pConstantBuffer, false);

		m_pGraphicsContext->SetIndexBuffer(m->GetIndexBuffer(), sizeOfIndices);
		m_pGraphicsContext->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
	}
}
