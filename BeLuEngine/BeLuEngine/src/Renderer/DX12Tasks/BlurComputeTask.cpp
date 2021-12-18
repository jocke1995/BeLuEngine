#include "stdafx.h"
#include "BlurComputeTask.h"

TODO("Remove this and put inside this class and rename class to BloomGraphicsPass");
#include "../Techniques/Bloom.h"

TODO("Abstract this");
#include "../PipelineState/ComputeState.h"

#include "../API/IGraphicsManager.h"
#include "../API/IGraphicsContext.h"
#include "../API/IGraphicsTexture.h"

BlurComputeTask::BlurComputeTask(Bloom* bloom, unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"BloomPass")
{
	// CSO
	m_PipelineStates.push_back(new ComputeState(L"ComputeBlurHorizontal.hlsl", L"blurHorizontalPSO"));
	m_PipelineStates.push_back(new ComputeState(L"ComputeBlurVertical.hlsl", L"blurVerticalPSO"));

	m_pTempBloom = bloom;

	m_HorizontalThreadGroupsX = static_cast<unsigned int>(ceilf(static_cast<float>(screenWidth) / m_ThreadsPerGroup));
	m_HorizontalThreadGroupsY = screenHeight;

	m_VerticalThreadGroupsX = screenWidth;
	m_VerticalThreadGroupsY = static_cast<unsigned int>(ceilf(static_cast<float>(screenHeight) / m_ThreadsPerGroup));
}

BlurComputeTask::~BlurComputeTask()
{

}

void BlurComputeTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(BlurCompute, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(true);

		// Descriptorheap indices for the textures to blur
		DescriptorHeapIndices dhIndices = {};
		// Horizontal pass
		dhIndices.index0 = m_pTempBloom->GetPingPongTexture(0)->GetShaderResourceHeapIndex();	// Read
		dhIndices.index1 = m_pTempBloom->GetPingPongTexture(1)->GetUnorderedAccessIndex();	// Write
		// Vertical pass
		dhIndices.index2 = m_pTempBloom->GetPingPongTexture(1)->GetShaderResourceHeapIndex();	// Read
		dhIndices.index3 = m_pTempBloom->GetPingPongTexture(0)->GetUnorderedAccessIndex();	// Write

		// Send the indices to gpu
		m_pGraphicsContext->Set32BitConstants(Constants_DH_Indices_B1, sizeof(DescriptorHeapIndices) / 4, &dhIndices, 0, true);

		// The resource to read (Resource Barrier)
		m_pGraphicsContext->ResourceBarrier(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// The resource to write (Resource Barrier)
		m_pGraphicsContext->ResourceBarrier(m_pTempBloom->GetPingPongTexture(1), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur horizontal
		m_pGraphicsContext->SetPipelineState(m_PipelineStates[0]->GetPSO());
		m_pGraphicsContext->Dispatch(m_HorizontalThreadGroupsX, m_HorizontalThreadGroupsY, 1);

		// The resource to write to (Resource Barrier)
		m_pGraphicsContext->ResourceBarrier(m_pTempBloom->GetPingPongTexture(1), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		// The resource to read (Resource Barrier)
		m_pGraphicsContext->ResourceBarrier(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Blur vertical
		m_pGraphicsContext->SetPipelineState(m_PipelineStates[1]->GetPSO());
		m_pGraphicsContext->Dispatch(m_VerticalThreadGroupsX, m_VerticalThreadGroupsY, 1);

		// Resource barrier back to original states
		m_pGraphicsContext->ResourceBarrier(m_pTempBloom->GetPingPongTexture(0), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	m_pGraphicsContext->End();
}
