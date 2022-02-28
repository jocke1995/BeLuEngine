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
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsPipelineState.h"

DebugTexturesPass::DebugTexturesPass(Mesh* fullScreenQuad)
	: GraphicsPass(L"DebugTexturesPass")
{
	m_pFullScreenQuad = fullScreenQuad;

	m_PipelineStates.resize(E_DEBUG_TEXTURE_VISUALIZATION::NUM_TEXTURES_TO_VISUALIZE - 1);

	// Init
	for (IGraphicsPipelineState* pso : m_PipelineStates)
		pso = nullptr;

	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_ALBEDO, L"ALBEDO");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_NORMAL, L"NORMAL");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_ROUGHNESS, L"ROUGHNESS");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_METALLIC, L"METALLIC");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_EMISSIVE, L"EMISSIVE");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_REFLECTION, L"REFLECTION");
	createStatePermutation(E_DEBUG_TEXTURE_VISUALIZATION_DEPTH, L"DEPTH");
}

DebugTexturesPass::~DebugTexturesPass()
{
}

void DebugTexturesPass::Execute()
{
	// If we're not visualizing anything, just return
	if (m_CurrentTextureToVisualize == E_DEBUG_TEXTURE_VISUALIZATION_FINALCOLOR)
		return;

	auto ResourceBarrierTextureToVisualize = [this](const E_DEBUG_TEXTURE_VISUALIZATION debugTexture)
	{
		if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_ALBEDO)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->gBufferAlbedo, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		else if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_NORMAL)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->gBufferNormal, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		else if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_ROUGHNESS || debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_METALLIC)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->gBufferMaterialProperties, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		else if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_EMISSIVE)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->gBufferEmissive, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		else if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_REFLECTION)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->reflectionTexture, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		else if (debugTexture == E_DEBUG_TEXTURE_VISUALIZATION_DEPTH)
			m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->mainDepthStencil, BL_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	};

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DebugPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetPipelineState(m_PipelineStates[m_CurrentTextureToVisualize]);
		m_pGraphicsContext->SetPrimitiveTopology(BL_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_pGraphicsContext->SetViewPort(m_CommonGraphicsResources->renderWidth, m_CommonGraphicsResources->renderHeight);
		m_pGraphicsContext->SetScizzorRect(m_CommonGraphicsResources->renderWidth, m_CommonGraphicsResources->renderHeight);

		// Just overwrite this texture with the debugTextureData
		m_pGraphicsContext->ResourceBarrier(m_CommonGraphicsResources->finalColorBuffer, BL_RESOURCE_STATE_RENDER_TARGET);
		m_pGraphicsContext->SetRenderTargets(1, &m_CommonGraphicsResources->finalColorBuffer, nullptr);

		// Set cbvs
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B3, m_CommonGraphicsResources->cbPerFrame, false);
		m_pGraphicsContext->SetConstantBuffer(RootParam_CBV_B4, m_CommonGraphicsResources->cbPerScene, false);

		ResourceBarrierTextureToVisualize((E_DEBUG_TEXTURE_VISUALIZATION)m_CurrentTextureToVisualize);

		// Draw a fullscreen quad 
		SlotInfo slotInfo = {};
		slotInfo.vertexDataIndex = m_pFullScreenQuad->GetVertexBuffer()->GetShaderResourceHeapIndex();
		m_pGraphicsContext->Set32BitConstants(Constants_SlotInfo_B0, sizeof(SlotInfo) / 4, &slotInfo, 0, false);

		m_pGraphicsContext->SetIndexBuffer(m_pFullScreenQuad->GetIndexBuffer(), m_pFullScreenQuad->GetSizeOfIndices());

		m_pGraphicsContext->DrawIndexedInstanced(m_pFullScreenQuad->GetNumIndices(), 1, 0, 0, 0);

	}
	m_pGraphicsContext->End();
}

void DebugTexturesPass::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

void DebugTexturesPass::AdvanceTextureToVisualize()
{
	m_CurrentTextureToVisualize +=(E_DEBUG_TEXTURE_VISUALIZATION)1;

	m_CurrentTextureToVisualize %= NUM_TEXTURES_TO_VISUALIZE;
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

void DebugTexturesPass::createStatePermutation(const E_DEBUG_TEXTURE_VISUALIZATION psoIndex, const LPCWSTR& define)
{
	PSODesc psoDesc = {};
	psoDesc.AddRenderTargetFormat(BL_FORMAT_R16G16B16A16_FLOAT);	// sceneColor (HDR-range)

	{
		DXILCompilationDesc shaderDesc = {};
		shaderDesc.defines.push_back(define);
		shaderDesc.filePath = L"DebugTexturePassVertex.hlsl";
		shaderDesc.shaderType = E_SHADER_TYPE::VS;
		psoDesc.AddShader(shaderDesc);
	}
	{
		DXILCompilationDesc shaderDesc = {};
		shaderDesc.defines.push_back(define);
		shaderDesc.filePath = L"DebugTexturePassPixel.hlsl";
		shaderDesc.shaderType = E_SHADER_TYPE::PS;
		psoDesc.AddShader(shaderDesc);
	}

	IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"DebugTexturesPass");
	m_PipelineStates[psoIndex] = iGraphicsPSO;
}
