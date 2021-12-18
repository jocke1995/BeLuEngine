#include "stdafx.h"
#include "TransparentRenderTask.h"

// Model info
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

TODO("Abstract this")
#include "../PipelineState/GraphicsState.h"

// Generic API
#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsBuffer.h"
#include "../API/IGraphicsTexture.h"
#include "../API/IGraphicsContext.h"

TransparentRenderTask::TransparentRenderTask()
	:GraphicsPass(L"TransparentPass")
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendFrontCull = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendBackCull = {};
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdBlendVector;

	gpsdBlendFrontCull.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdBlendFrontCull.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdBlendFrontCull.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdBlendFrontCull.SampleDesc.Count = 1;
	gpsdBlendFrontCull.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdBlendFrontCull.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdBlendFrontCull.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC blendRTdesc{};
	blendRTdesc.BlendEnable = true;
	blendRTdesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendRTdesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendRTdesc.BlendOp = D3D12_BLEND_OP_ADD;
	blendRTdesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendRTdesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendRTdesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendRTdesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		gpsdBlendFrontCull.BlendState.RenderTarget[i] = blendRTdesc;
	}

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsdBlend = {};
	dsdBlend.DepthEnable = true;
	dsdBlend.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsdBlend.DepthFunc = D3D12_COMPARISON_FUNC_LESS;	// Om pixels depth är lägre än den gamla så ritas den nya ut

	// DepthStencil
	dsdBlend.StencilEnable = false;
	dsdBlend.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsdBlend.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC blendStencilOP{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsdBlend.FrontFace = blendStencilOP;
	dsdBlend.BackFace = blendStencilOP;

	gpsdBlendFrontCull.DepthStencilState = dsdBlend;
	gpsdBlendFrontCull.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// ------------------------ BLEND ---------------------------- BACKCULL

	gpsdBlendBackCull.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdBlendBackCull.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdBlendBackCull.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdBlendBackCull.SampleDesc.Count = 1;
	gpsdBlendBackCull.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdBlendBackCull.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdBlendBackCull.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdBlendBackCull.BlendState.RenderTarget[i] = blendRTdesc;

	// DepthStencil
	dsdBlend.StencilEnable = false;
	dsdBlend.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	gpsdBlendBackCull.DepthStencilState = dsdBlend;
	gpsdBlendBackCull.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_PipelineStates.push_back(new GraphicsState(L"TransparentTextureVertex.hlsl", L"TransparentTexturePixel.hlsl", &gpsdBlendFrontCull, L"BlendFrontCull"));
	m_PipelineStates.push_back(new GraphicsState(L"TransparentTextureVertex.hlsl", L"TransparentTexturePixel.hlsl", &gpsdBlendBackCull, L"BlendBackCull"));
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

		m_pGraphicsContext->SetRenderTargets(1, &m_GraphicTextures["finalColorBuffer"], m_GraphicTextures["mainDepthStencilBuffer"]);

		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_GraphicBuffers["cbPerFrame"], false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_GraphicBuffers["cbPerScene"], false);
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
					m_pGraphicsContext->SetPipelineState(m_PipelineStates[k]->GetPSO());
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
