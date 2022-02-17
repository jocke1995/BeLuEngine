#include "stdafx.h"
#include "OutliningRenderTask.h"

#include "../Renderer/Camera/PerspectiveCamera.h"
#include "../Renderer/Geometry/Mesh.h"

// Components
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"


OutliningRenderTask::OutliningRenderTask()
	:GraphicsPass(L"OutliningPass")
{
	// Init with nullptr
	Clear();

	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ALL, BL_COMPARISON_FUNC_LESS);

	const BL_DEPTH_STENCILOP_DESC stencilNotEqual =
	{
		BL_STENCIL_OP_KEEP, BL_STENCIL_OP_KEEP, BL_STENCIL_OP_REPLACE,
		BL_COMPARISON_FUNC_NOT_EQUAL
	};
	psoDesc.SetStencilDesc(0xff, 0x00, stencilNotEqual, stencilNotEqual);

	psoDesc.AddShader(L"OutlinedVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"OutlinedPixel.hlsl", E_SHADER_TYPE::PS);

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"OutlinedRenderPass");
	m_PipelineStates.push_back(iGraphicsPSO);
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
			m_pGraphicsContext->ClearDepthTexture(m_CommonGraphicsResources->mainDepthStencil, false, 0.0f, true, 0); 
			goto End;	// Hack
		}
		// else continue as usual

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, BL_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, BL_RESOURCE_STATE_DEPTH_WRITE);

		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, m_CommonGraphicsResources->mainDepthStencil);

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
