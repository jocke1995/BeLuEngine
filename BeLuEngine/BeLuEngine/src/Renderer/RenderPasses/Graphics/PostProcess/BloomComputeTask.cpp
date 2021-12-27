#include "stdafx.h"
#include "BloomComputeTask.h"

#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

enum E_BLOOM_PIPELINE_STATES
{
	Bloom_PreFilterState,
	Bloom_BlurHorizontalState,
	Bloom_BlurVerticalState,
	Bloom_CompositeState,
	Bloom_NUM_PIPELINE_STATES
};
BloomComputePass::BloomComputePass(unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"BloomPass")
{
#pragma region PipelineStates
	m_PipelineStates.resize(E_BLOOM_PIPELINE_STATES::Bloom_NUM_PIPELINE_STATES);
	// Prefilter
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/PreFilterCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"PreFilterBloomPSO");
		m_PipelineStates[Bloom_PreFilterState] = iGraphicsPSO;
	}

	// Blur horizontal
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/BlurHorizontalCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"blurHorizontalPSO");
		m_PipelineStates[Bloom_BlurHorizontalState] = iGraphicsPSO;
	}

	// Blur vertical
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/BlurVerticalCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"blurVerticalPSO");
		m_PipelineStates[Bloom_BlurVerticalState] = iGraphicsPSO;
	}

	// Composite
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/CompositeCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"CompositePSO");
		m_PipelineStates[Bloom_CompositeState] = iGraphicsPSO;
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

		// Prefilter and Downscale
		{
			ScopedPixEvent(Prefilter, m_pGraphicsContext);
			dhIndices.index0 = finalColorTexture->GetShaderResourceHeapIndex();		// Read and filter bright areas from this texture
			dhIndices.index1 = m_PingPongTextures[0]->GetUnorderedAccessIndex();	// Write bright areas into this texture (in 1 mip lower)

			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_PreFilterState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			// Make sure it's completed before reading from it in the following passes!
			m_pGraphicsContext->UAVBarrier(m_PingPongTextures[0]);

			TODO("Resource Barriers on finalColorBuffer?");
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// Blur Horizontal
		{
			ScopedPixEvent(HorizontalBlur, m_pGraphicsContext);

			dhIndices.index0 = m_PingPongTextures[0]->GetShaderResourceHeapIndex();	// Read
			dhIndices.index1 = m_PingPongTextures[1]->GetUnorderedAccessIndex();	// Write


			// The resource to read (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			// Blur horizontal
			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_BlurHorizontalState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			m_pGraphicsContext->UAVBarrier(m_PingPongTextures[1]);

			// The resource to read (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
		

		// Blur vertical
		{
			ScopedPixEvent(VerticalBlur, m_pGraphicsContext);

			dhIndices.index2 = m_PingPongTextures[1]->GetShaderResourceHeapIndex();	// Read
			dhIndices.index3 = m_PingPongTextures[0]->GetUnorderedAccessIndex();	// Write

			// The resource to write (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_BlurVerticalState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

			// The resource to write (Resource Barrier)
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// Composite
		// Adding the blur color to the sceneColor
		{
			ScopedPixEvent(Composite, m_pGraphicsContext);

			dhIndices.index0 = m_PingPongTextures[0]->GetShaderResourceHeapIndex();	// Read
			dhIndices.index1 = finalColorTexture->GetShaderResourceHeapIndex();		// Read
			dhIndices.index2 = finalColorTexture->GetUnorderedAccessIndex();		// Write

			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_CompositeState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			m_pGraphicsContext->UAVBarrier(finalColorTexture);

			// Transfer back to original state
			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
		}
	}
	m_pGraphicsContext->End();
}
