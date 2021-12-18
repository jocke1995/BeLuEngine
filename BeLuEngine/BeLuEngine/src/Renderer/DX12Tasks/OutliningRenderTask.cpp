#include "stdafx.h"
#include "OutliningRenderTask.h"

// DX12 Specifics
#include "../PipelineState/GraphicsState.h"

#include "../Camera/PerspectiveCamera.h"
#include "../Renderer/Geometry/Mesh.h"

// Componennt
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsBuffer.h"
#include "../API/IGraphicsTexture.h"
#include "../API/IGraphicsContext.h"


OutliningRenderTask::OutliningRenderTask()
	:GraphicsPass(L"OutliningPass")
{
	// Init with nullptr
	Clear();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdModelOutlining = {};
	gpsdModelOutlining.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget (TODO: Formats are way to big atm)
	gpsdModelOutlining.NumRenderTargets = 1;
	gpsdModelOutlining.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdModelOutlining.SampleDesc.Count = 1;
	gpsdModelOutlining.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdModelOutlining.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdModelOutlining.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdModelOutlining.RasterizerState.DepthBias = 0;
	gpsdModelOutlining.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdModelOutlining.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdModelOutlining.RasterizerState.FrontCounterClockwise = false;

	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = true;	// Maybe enable if we dont want the object to "highlight" through other objects
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// DepthStencil
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0x00;
	const D3D12_DEPTH_STENCILOP_DESC stencilNotEqual =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE,
		D3D12_COMPARISON_FUNC_NOT_EQUAL
	};
	dsd.FrontFace = stencilNotEqual;
	dsd.BackFace = stencilNotEqual;

	gpsdModelOutlining.DepthStencilState = dsd;
	gpsdModelOutlining.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	m_PipelineStates.push_back(new GraphicsState(L"OutlinedVertex.hlsl", L"OutlinedPixel.hlsl", &gpsdModelOutlining, L"OutlinedRenderPass"));
}

OutliningRenderTask::~OutliningRenderTask()
{
}

void OutliningRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(OutliningPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		// Check if there is an object to outline
		if (m_ObjectToOutline.first == nullptr)
		{
			m_pGraphicsContext->ClearDepthTexture(m_GraphicTextures["mainDepthStencilBuffer"], false, 0.0f, true, 0); 
			goto End;	// Hack
		}
		// else continue as usual

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());
		m_pGraphicsContext->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->SetRenderTargets(1, &m_GraphicTextures["finalColorBuffer"], m_GraphicTextures["mainDepthStencilBuffer"]);

		const DirectX::XMMATRIX* viewProjMatTrans = m_pCamera->GetViewProjectionTranposed();
		// Draw for every mesh in the model
		for (int i = 0; i < m_ObjectToOutline.first->GetNrOfMeshes(); i++)
		{
			Mesh* m = m_ObjectToOutline.first->GetMeshAt(i);
			Transform* t = m_ObjectToOutline.second->GetTransform();
			Transform newScaledTransform = *t;
			newScaledTransform.IncreaseScaleByPercent(0.02f);
			newScaledTransform.UpdateWorldMatrix();

			component::ModelComponent* mc = m_ObjectToOutline.first;

			unsigned int num_Indices = m->GetNumIndices();
			const SlotInfo* info = mc->GetSlotInfoAt(i);

			DirectX::XMMATRIX w_wvp[2] = {};
			w_wvp[0] = *newScaledTransform.GetWorldMatrixTransposed();
			w_wvp[1] = (*viewProjMatTrans) * w_wvp[0];

			m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, info, 0, false);
			m_pGraphicsContext->SetConstantBufferDynamicData(RootParam_CBV_B2, sizeof(DirectX::XMMATRIX) * 2, &w_wvp, false);

			m_pGraphicsContext->SetIndexBuffer(m->GetIndexBuffer(), m->GetSizeOfIndices());

			m_pGraphicsContext->SetStencilRef(1);
			m_pGraphicsContext->DrawIndexedInstanced(num_Indices, 1, 0, 0, 0);
		}
	}
End:
	m_pGraphicsContext->End();
}

void OutliningRenderTask::SetObjectToOutline(std::pair<component::ModelComponent*, component::TransformComponent*>* objectToOutline)
{
	m_ObjectToOutline = *objectToOutline;
}

void OutliningRenderTask::Clear()
{
	m_ObjectToOutline.first = nullptr;
	m_ObjectToOutline.second = nullptr;
}
