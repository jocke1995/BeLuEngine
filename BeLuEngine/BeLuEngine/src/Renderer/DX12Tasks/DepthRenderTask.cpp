#include "stdafx.h"
#include "DepthRenderTask.h"

// Model info
#include "../Geometry/Mesh.h"
#include "../Geometry/Transform.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

TODO("Abstract this")
#include "../PipelineState/GraphicsState.h"

// Generic API
#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsContext.h"

DepthRenderTask::DepthRenderTask()
	: GraphicsPass(L"DepthPrePass")
{
	TODO("Abstract state");
	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDepthPrePass = {};
	gpsdDepthPrePass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsdDepthPrePass.NumRenderTargets = 0;
	gpsdDepthPrePass.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsdDepthPrePass.SampleDesc.Count = 1;
	gpsdDepthPrePass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDepthPrePass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDepthPrePass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDepthPrePass.RasterizerState.DepthBias = 0;
	gpsdDepthPrePass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdDepthPrePass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdDepthPrePass.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC depthPrePassRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDepthPrePass.BlendState.RenderTarget[i] = depthPrePassRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC depthPrePassDsd = {};
	depthPrePassDsd.DepthEnable = true;
	depthPrePassDsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthPrePassDsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// DepthStencil
	depthPrePassDsd.StencilEnable = false;
	gpsdDepthPrePass.DepthStencilState = depthPrePassDsd;
	gpsdDepthPrePass.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_PipelineStates.push_back(new GraphicsState(L"DepthVertex.hlsl", L"DepthPixel.hlsl", &gpsdDepthPrePass, L"DepthPrePass"));
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

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());
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
