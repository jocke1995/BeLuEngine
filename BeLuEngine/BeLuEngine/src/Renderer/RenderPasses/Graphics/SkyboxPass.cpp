#include "stdafx.h"
#include "SkyboxPass.h"

#include "../ECS/Components/SkyboxComponent.h"

// Generic API
#include "../../API/Interface/IGraphicsManager.h"
#include "../../API/Interface/IGraphicsBuffer.h"
#include "../../API/Interface/IGraphicsTexture.h"
#include "../../API/Interface/IGraphicsContext.h"
#include "../../API/Interface/IGraphicsPipelineState.h"

SkyboxPass::SkyboxPass()
	: GraphicsPass(L"GeometryPass")
{
	PSODesc psoDesc = {};
	// RenderTarget (TODO: Formats are way to big atm)
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);

	psoDesc.SetDepthStencilFormat(BL_FORMAT_D24_UNORM_S8_UINT);
	psoDesc.SetDepthDesc(BL_DEPTH_WRITE_MASK_ALL, BL_COMPARISON_FUNC_LESS_EQUAL);


	psoDesc.AddShader(L"SkyboxVertex.hlsl", E_SHADER_TYPE::VS);
	psoDesc.AddShader(L"SkyboxVPixel.hlsl", E_SHADER_TYPE::PS);
	
	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"SkyboxPass");
	m_PipelineStates.push_back(iGraphicsPSO);
}

SkyboxPass::~SkyboxPass()
{
}

void SkyboxPass::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(GBufferPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		TODO("Don't hardcode the sizes");
		m_pGraphicsContext->SetViewPort(1280, 720);
		m_pGraphicsContext->SetScizzorRect(1280, 720);
	}
	m_pGraphicsContext->End();
}

void SkyboxPass::SetSkybox(component::SkyboxComponent* skyboxComponent)
{
	m_pSkyboxComponent = skyboxComponent;
}
