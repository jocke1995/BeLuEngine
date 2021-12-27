#include "stdafx.h"
#include "BloomComputeTask.h"

#include "../Renderer/API/IGraphicsManager.h"
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsPipelineState.h"

enum E_BLOOM_PIPELINE_STATES
{
	Bloom_FilterAndDownSampleState,
	Bloom_DownSampleAndBlurHorizontalState,
	Bloom_BlurVerticalState,
	Bloom_UpSampleAndCombineState,
	Bloom_CompositeState,
	Bloom_NUM_PIPELINE_STATES
};

constexpr unsigned int g_NumMips = 6;

BloomComputePass::BloomComputePass(unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"BloomPass")
{
#pragma region PipelineStates
	m_PipelineStates.resize(E_BLOOM_PIPELINE_STATES::Bloom_NUM_PIPELINE_STATES);

	// Filter and Downsample
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/FilterAndDownSampleCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Bloom_FilterAndDownSamplePSO");
		m_PipelineStates[Bloom_FilterAndDownSampleState] = iGraphicsPSO;
	}

	// Downsample and Blur horizontal
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/DownSampleAndBlurHorizontalCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Bloom_DownSampleAndBlurHorizontalPSO");
		m_PipelineStates[Bloom_DownSampleAndBlurHorizontalState] = iGraphicsPSO;
	}

	// Blur vertical
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/BlurVerticalCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Bloom_BlurVerticalPSO");
		m_PipelineStates[Bloom_BlurVerticalState] = iGraphicsPSO;
	}

	// UpSample and Combine
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/UpSampleAndCombineCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Bloom_UpSampleAndCombinePSO");
		m_PipelineStates[Bloom_UpSampleAndCombineState] = iGraphicsPSO;
	}

	// Composite
	{
		PSODesc psoDesc = {};
		psoDesc.AddShader(L"PostProcess/FinalCompositeCompute.hlsl", E_SHADER_TYPE::CS);
		IGraphicsPipelineState* iGraphicsPSO = IGraphicsPipelineState::Create(psoDesc, L"Bloom_FinalCompositePSO");
		m_PipelineStates[Bloom_CompositeState] = iGraphicsPSO;
	}

#pragma endregion

#pragma region PingPongTextures
	m_PingPongTextures[0] = IGraphicsTexture::Create();
	m_PingPongTextures[0]->CreateTexture2D(
		screenWidth, screenHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture0", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, g_NumMips);


	m_PingPongTextures[1] = IGraphicsTexture::Create();
	m_PingPongTextures[1]->CreateTexture2D(
		screenWidth, screenHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::UnorderedAccess | F_TEXTURE_USAGE::ShaderResource,
		L"PingPongTexture1", D3D12_RESOURCE_STATE_UNORDERED_ACCESS, g_NumMips);

#pragma endregion

	m_ScreenWidth = screenWidth;
	m_ScreenHeight = screenHeight;

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
		// Filter brightAreas and Downsample
		{
			ScopedPixEvent(Filter_Downsample, m_pGraphicsContext);
			
			// DownsampleSize is the next mip (A.K.A half width and height)
			unsigned int blurWidth = m_ScreenWidth >> 1;
			unsigned int blurHeight = m_ScreenHeight >> 1;

			dhIndices = {};
			dhIndices.index0 = finalColorTexture->GetShaderResourceHeapIndex();		// Read and filter bright areas from this texture
			dhIndices.index1 = m_PingPongTextures[0]->GetUnorderedAccessIndex(1);	// Write bright areas into this texture

			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_FilterAndDownSampleState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);

			unsigned int threadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(blurWidth) / m_ThreadsPerGroup));
			unsigned int threadGroupsY = blurHeight;
			m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);

			// Make sure it's completed before reading from it in the following passes!
			m_pGraphicsContext->UAVBarrier(m_PingPongTextures[0]);

			TODO("Resource Barriers on finalColorBuffer?");

			// The resource to read
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// DownFilter and Blur
		for(unsigned int mipLevel = 1; mipLevel < g_NumMips - 1; mipLevel++)
		{
			ScopedPixEvent(DownSample_Blur, m_pGraphicsContext);

			unsigned int blurWidth	= m_ScreenWidth >> mipLevel;
			unsigned int blurHeight = m_ScreenHeight >> mipLevel;

			// Downsample and Blur Horizontal
			{
				ScopedPixEvent(HorizontalBlur, m_pGraphicsContext);

				dhIndices = {};
				dhIndices.index0 = m_PingPongTextures[0]->GetShaderResourceHeapIndex(mipLevel);		// Read
				dhIndices.index1 = m_PingPongTextures[1]->GetUnorderedAccessIndex(mipLevel + 1);	// Write
				dhIndices.index2 = mipLevel + 1; // Mip to write to

				// Blur horizontal
				m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_DownSampleAndBlurHorizontalState]);
				m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);

				unsigned int threadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(blurWidth) / m_ThreadsPerGroup));
				unsigned int threadGroupsY = blurHeight;
				m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);

				m_pGraphicsContext->UAVBarrier(m_PingPongTextures[1]);

				// The resource to read (Resource Barrier)
				m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			}


			// Blur Vertical (using the same outputSize as inputSize)
			{
				ScopedPixEvent(VerticalBlur, m_pGraphicsContext);

				dhIndices = {};
				dhIndices.index0 = m_PingPongTextures[1]->GetShaderResourceHeapIndex(mipLevel + 1);	// Read
				dhIndices.index1 = m_PingPongTextures[0]->GetUnorderedAccessIndex(mipLevel + 1);	// Write
				dhIndices.index2 = mipLevel + 1; // Mip to write to

				// The resource to write (Resource Barrier)
				m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

				m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_BlurVerticalState]);
				m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);

				unsigned int threadGroupsX = blurWidth;
				unsigned int threadGroupsY = static_cast<unsigned int>(ceilf(static_cast<float>(blurHeight) / m_ThreadsPerGroup));
				m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);
				m_pGraphicsContext->UAVBarrier(m_PingPongTextures[0]);

				// The resource to read (Resource Barrier)
				m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				// The resource to write (Resource Barrier)
				m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			}
		}

		// UpSample and Combine
		IGraphicsTexture* inputTexture = m_PingPongTextures[0];
		IGraphicsTexture* outputTexture = m_PingPongTextures[1];
		for (unsigned int mipLevel = g_NumMips - 1; mipLevel >= 1; mipLevel--)
		{
			ScopedPixEvent(UpSample_Combine, m_pGraphicsContext);
			
			// Write to 1 mip lower then we read from
			unsigned int mipToWriteTo = mipLevel - 1;
			unsigned int textureWidth = m_ScreenWidth >> mipToWriteTo;
			unsigned int textureHeight = m_ScreenHeight >> mipToWriteTo;

			unsigned int threadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(textureWidth) / m_ThreadsPerGroup));
			unsigned int threadGroupsY = textureHeight;

			DescriptorHeapIndices dhIndices1 = {};
			dhIndices1.index0 = inputTexture->GetShaderResourceHeapIndex(mipLevel);	// Read  (Downscaled)
			dhIndices1.index1 = outputTexture->GetShaderResourceHeapIndex(mipToWriteTo);	// Read  (Current Size)
			dhIndices1.index2 = outputTexture->GetUnorderedAccessIndex(mipToWriteTo);	// Write (Current Size)
			dhIndices1.index3 = mipToWriteTo;

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_UpSampleAndCombineState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices1, 0, true);
			m_pGraphicsContext->Dispatch(threadGroupsX, threadGroupsY, 1);
			m_pGraphicsContext->UAVBarrier(outputTexture);

			// Swap input and output textures
			IGraphicsTexture* temp = inputTexture;
			inputTexture = outputTexture;
			outputTexture = temp;

			m_pGraphicsContext->ResourceBarrier(inputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_pGraphicsContext->ResourceBarrier(outputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		// Composite
		// Adding the blur color to the sceneColor
		{
			ScopedPixEvent(Composite, m_pGraphicsContext);

			DescriptorHeapIndices dhIndices2 = {};
			dhIndices2.index0 = inputTexture->GetShaderResourceHeapIndex(0);		 // Read
			dhIndices2.index1 = finalColorTexture->GetShaderResourceHeapIndex(0); // Read
			dhIndices2.index2 = finalColorTexture->GetUnorderedAccessIndex(0);	 // Write
			dhIndices2.index3 = g_NumMips;		// MipTemp

			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			m_pGraphicsContext->SetPipelineState(m_PipelineStates[Bloom_CompositeState]);
			m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices2, 0, true);
			m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

			m_pGraphicsContext->UAVBarrier(finalColorTexture);

			// Transfer back to original state
			m_pGraphicsContext->ResourceBarrier(finalColorTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[0], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			m_pGraphicsContext->ResourceBarrier(m_PingPongTextures[1], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		}
	}
	m_pGraphicsContext->End();
}