#include "stdafx.h"
#include "SkyboxPass.h"

#include "../ECS/Components/SkyboxComponent.h"

#include "../Renderer/Geometry/Model.h"
#include "../Renderer/Geometry/Mesh.h"

// Generic API
#include "../../API/Interface/IGraphicsManager.h"
#include "../../API/Interface/IGraphicsBuffer.h"
#include "../../API/Interface/IGraphicsTexture.h"
#include "../../API/Interface/IGraphicsContext.h"
#include "../../API/Interface/IGraphicsPipelineState.h"

#include "../Misc/AssetLoader.h"

SkyboxPass::SkyboxPass()
	: GraphicsPass(L"GeometryPass")
{
	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ALL, BL_COMPARISON_FUNC_LESS_EQUAL);


	psoDesc.AddShader(L"SkyboxVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"SkyboxPixel.hlsl", E_SHADER_TYPE::PS);
	
	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"SkyboxPass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

SkyboxPass::~SkyboxPass()
{
}

void SkyboxPass::Execute()
{
	// Just check if we have a skybox set for now.
	// Handle this better in the future..
	BL_ASSERT(m_pSkyboxComponent);

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(Skybox, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);

		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_CommonGraphicsResources->cbPerFrame, false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_CommonGraphicsResources->cbPerScene, false);

		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, m_CommonGraphicsResources->mainDepthStencil);

		// Draw a 1x1x1 cube 
		BL_ASSERT(m_pSkyboxComponent->m_pCube);	// This has to be set during InitSkyboxComponent from the Renderer

		Mesh* cube = m_pSkyboxComponent->m_pCube->GetMeshAt(0);
		SlotInfo slotInfo = {};

		slotInfo.vertexDataIndex = cube->GetVertexBuffer()->GetShaderResourceHeapIndex();
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);

		m_pGraphicsContext->SetIndexBuffer(cube->GetIndexBuffer(), cube->GetSizeOfIndices());

		m_pGraphicsContext->DrawIndexedInstanced(cube->GetNumIndices(), 1, 0, 0, 0);
	}
	m_pGraphicsContext->End();
}

void SkyboxPass::SetSkybox(component::SkyboxComponent* skyboxComponent)
{
	m_pSkyboxComponent = skyboxComponent;
}
