#include "stdafx.h"
#include "BlurComputeTask.h"

#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

BloomComputePass::BloomComputePass(unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"BloomPass")
{
#pragma region PipelineStates
	// Prefilter
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"ComputePreFilterBloom.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"PreFilterBloomPSO");
		m_PipelineStates.push_back(iGraphicsPSO);
	}

	// Blur horizontal
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"ComputeBlurHorizontal.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"blurHorizontalPSO");
		m_PipelineStates.push_back(iGraphicsPSO);
	}

	// Blur vertical
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"ComputeBlurVertical.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"blurVerticalPSO");
		m_PipelineStates.push_back(iGraphicsPSO);
	}

#pragma endregion

#pragma region PingPongTextures
	m_PingPongTextures[0] = IGraphicsTexture::Create();
	m_PingPongTextures[0]->CreateTexture2D(
		screenWidth, screenHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture0", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


	m_PingPongTextures[1] = IGraphicsTexture::Create();
	m_PingPongTextures[1]->CreateTexture2D(
		screenWidth, screenHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture1", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	m_PreFilterTexture = IGraphicsTexture::Create();
	m_PreFilterTexture->CreateTexture2D(
		screenWidth, screenHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::ShaderResource,
		L"PreFilterTextureBloom", D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
#pragma endregion

	m_HorizontalThreadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(screenWidth) / m_ThreadsPerGroup));
	m_HorizontalThreadGroupsY = screenHeight;

	m_VerticalThreadGroupsX = screenWidth;
	m_VerticalThreadGroupsY = static_cast<unsigned int>(ceilf(static_cast<float>(screenHeight) / m_ThreadsPerGroup));
}

BloomComputePass::~BloomComputePass()
{
	for (unsigned int i = 0; i < 2; i++)
	{
		BL_SAFE_DELETE(m_PingPongTextures[i]);
	}

	BL_SAFE_DELETE(m_PreFilterTexture);
}

void BloomComputePass::Execute()
{
	IGraphicsTexture* finalColorTexture = m_GraphicTextures["finalColorBuffer"];
	BL_ASSERT(finalColorTexture);

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(Bloom, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(true);

		DescriptorHeapIndices dhIndices = {};

		// Prefilter
		{
			ScopedPixEvent(Prefilter, m_pGraphicsContext);
			m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]);
			dhIndices.index0 = finalColorTexture->GetShaderResourceHeapIndex();	// Read and filter bright areas from this texture
			dhIndices.index1 = m_PreFilterTexture->GetUnorderedAccessIndex();	// Write bright areas into this texture

			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			// Make sure it's completed before reading from it in the following passes!
			m_pGraphicsContext->UAVBarrier(m_PreFilterTexture);

			TODO("Resource Barriers on finalColorBuffer?");
			m_pGraphicsContext->ResourceBarrier(m_PreFilterTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// Blur Horizontal
		{
			ScopedPixEvent(HorizontalBlur, m_pGraphicsContext);
			// Descriptorheap indices for the textures to blur
			dhIndices.index0 = m_PreFilterTexture->GetShaderResourceHeapIndex();	// Read
			dhIndices.index1 = m_PingPongTextures[0]->GetUnorderedAccessIndex();	// Write

			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);

			// The resource to read (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			// Blur horizontal
			m_pGraphicsContext->SetPipelineState(m_PipelineStates[1]);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			m_pGraphicsContext->UAVBarrier(m_PingPongTextures[1]);

			m_pGraphicsContext->ResourceBarrier(m_PreFilterTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			// The resource to read (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		

		// Blur vertical
		{
			ScopedPixEvent(VerticalBlur, m_pGraphicsContext);
			// Vertical pass
			dhIndices.index2 = m_PingPongTextures[0]->GetShaderResourceHeapIndex();	// Read
			dhIndices.index3 = finalColorTexture->GetUnorderedAccessIndex();	// Write

			TODO("ResourceBarrier on FinalColorTexture");

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[2]);

			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

			m_pGraphicsContext->UAVBarrier(finalColorTexture);
		}
	}
	m_pGraphicsContext->End();
}
