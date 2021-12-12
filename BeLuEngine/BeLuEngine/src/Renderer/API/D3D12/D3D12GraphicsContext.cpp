#include "stdafx.h"
#include "D3D12GraphicsContext.h"

TODO("Remove");
#include "../Renderer/DescriptorHeap.h"

#include "D3D12GraphicsManager.h"
#include "D3D12GraphicsTexture.h"
#include "D3D12GraphicsBuffer.h"

D3D12GraphicsContext::D3D12GraphicsContext(const std::wstring& name)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device = graphicsManager->GetDevice();

#ifdef DEBUG
	m_Name = name;
#endif
	
	HRESULT hr;
#pragma region CreateCommandInterface
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocators[i]));

		if (!D3D12GraphicsManager::SucceededHRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to Create CommandAllocator: %S\n", name);
		}

		std::wstring counter = std::to_wstring(i);
		hr = m_pCommandAllocators[i]->SetName((name + L"_CmdAlloc_" + counter).c_str());
		
		if (!D3D12GraphicsManager::SucceededHRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to SetName on commandAllocator: %S\n", name);
		}
	}

	hr = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pCommandAllocators[0],
		nullptr,
		IID_PPV_ARGS(&m_pCommandList));

	hr = m_pCommandList->SetName((name + L"_CmdList").c_str());
	if (!D3D12GraphicsManager::SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to Create CommandList: %S\n", name);
	}

	hr = m_pCommandList->Close();
	if (!D3D12GraphicsManager::SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to SetName on CommandList: %S\n", name);
	}

#pragma endregion
}

D3D12GraphicsContext::~D3D12GraphicsContext()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pCommandAllocators[i]);
	}
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pCommandList);
}

void D3D12GraphicsContext::Begin()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	unsigned int index = graphicsManager->GetCommandInterfaceIndex();

	// Reset commandinterface
	D3D12GraphicsManager::SucceededHRESULT(m_pCommandAllocators[index]->Reset());
	D3D12GraphicsManager::SucceededHRESULT(m_pCommandList->Reset(m_pCommandAllocators[index], NULL));
}

void D3D12GraphicsContext::SetupBindings(bool isComputePipeline)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	// DescriptorHeap
	ID3D12DescriptorHeap* d3d12DescriptorHeap = graphicsManager->GetMainDescriptorHeap()->GetID3D12DescriptorHeap();
	m_pCommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	// Rootsignature and DescriptorTables
	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootSignature(graphicsManager->GetGlobalRootSignature());

		m_pCommandList->SetComputeRootDescriptorTable(dtSRV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetComputeRootDescriptorTable(dtCBV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetComputeRootDescriptorTable(dtUAV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
	else
	{
		m_pCommandList->SetGraphicsRootSignature(graphicsManager->GetGlobalRootSignature());

		m_pCommandList->SetGraphicsRootDescriptorTable(dtSRV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetGraphicsRootDescriptorTable(dtCBV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		m_pCommandList->SetGraphicsRootDescriptorTable(dtUAV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}
}

void D3D12GraphicsContext::End()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12GraphicsManager::SucceededHRESULT(m_pCommandList->Close());
}

void D3D12GraphicsContext::SetPipelineState(ID3D12PipelineState* pso)
{
	BL_ASSERT(pso);

	m_pCommandList->SetPipelineState(pso);
}

void D3D12GraphicsContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop)
{
	BL_ASSERT(primTop != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

	m_pCommandList->IASetPrimitiveTopology(primTop);
}

void D3D12GraphicsContext::SetViewPort(unsigned int width, unsigned int height, float topLeftX, float topLeftY, float minDepth, float maxDepth)
{
	D3D12_VIEWPORT viewPort = {};
	viewPort.Width = width;
	viewPort.Height = height;
	viewPort.TopLeftX = topLeftX;
	viewPort.TopLeftY = topLeftY;
	viewPort.MinDepth = minDepth;
	viewPort.MaxDepth = maxDepth;

	m_pCommandList->RSSetViewports(1, &viewPort);
}

void D3D12GraphicsContext::SetScizzorRect(unsigned int right, unsigned int bottom, float left, unsigned int top)
{
	D3D12_RECT rect = {};
	rect.right = right;
	rect.bottom = bottom;
	rect.left = left;
	rect.top = top;

	m_pCommandList->RSSetScissorRects(1, &rect);
}

void D3D12GraphicsContext::ClearDepthTexture(IGraphicsTexture* depthTexture, float depthValue, bool clearStencil, unsigned int stencilValue)
{
	BL_ASSERT(depthTexture);

	D3D12GraphicsTexture* d3d12DepthTexture = static_cast<D3D12GraphicsTexture*>(depthTexture);
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = graphicsManager->GetDSVDescriptorHeap()->GetCPUHeapAt(d3d12DepthTexture->GetDepthStencilIndex());

	D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAG_DEPTH;
	clearFlags |= clearStencil ? D3D12_CLEAR_FLAG_STENCIL : (D3D12_CLEAR_FLAGS)0;

	m_pCommandList->ClearDepthStencilView(cpuHandle, clearFlags, depthValue, stencilValue, 0, nullptr);
}

void D3D12GraphicsContext::ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4])
{
	BL_ASSERT(renderTargetTexture);

	D3D12GraphicsTexture* d3d12RenderTargetTexture = static_cast<D3D12GraphicsTexture*>(renderTargetTexture);
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = graphicsManager->GetRTVDescriptorHeap()->GetCPUHeapAt(d3d12RenderTargetTexture->GetRenderTargetHeapIndex());
	m_pCommandList->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void D3D12GraphicsContext::SetRenderTargets(unsigned int numRenderTargets, IGraphicsTexture* renderTargetTextures[], IGraphicsTexture* depthTexture)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12_CPU_DESCRIPTOR_HANDLE cpuRTVHandles[8] = {};
	for (unsigned int i = 0; i < numRenderTargets; i++)
	{
		unsigned int rtvIndex = static_cast<D3D12GraphicsTexture*>(renderTargetTextures[i])->GetRenderTargetHeapIndex();
		cpuRTVHandles[i] = graphicsManager->GetRTVDescriptorHeap()->GetCPUHeapAt(rtvIndex);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuDSVHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE* pcpuDSVHandle = nullptr;
	if (depthTexture != nullptr)
	{
		unsigned int dsvIndex = static_cast<D3D12GraphicsTexture*>(depthTexture)->GetDepthStencilIndex();
		cpuDSVHandle = graphicsManager->GetDSVDescriptorHeap()->GetCPUHeapAt(dsvIndex);
		pcpuDSVHandle = &cpuDSVHandle;
	}

	m_pCommandList->OMSetRenderTargets(numRenderTargets, cpuRTVHandles, false, pcpuDSVHandle);
}

void D3D12GraphicsContext::SetShaderResourceView(unsigned int rootParamSlot, IGraphicsTexture* graphicsTexture, bool isComputePipeline)
{
	BL_ASSERT(graphicsTexture);

	D3D12GraphicsTexture* d3d12GraphicsTexture = static_cast<D3D12GraphicsTexture*>(graphicsTexture);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootShaderResourceView(rootParamSlot, d3d12GraphicsTexture->m_pResource->GetGPUVirtualAddress());
	}
	else
	{
		m_pCommandList->SetGraphicsRootShaderResourceView(rootParamSlot, d3d12GraphicsTexture->m_pResource->GetGPUVirtualAddress());
	}
}

void D3D12GraphicsContext::SetShaderResourceView(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline)
{
}

void D3D12GraphicsContext::SetConstantBuffer(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline)
{
	BL_ASSERT(graphicsBuffer);

	D3D12GraphicsBuffer* d3d12GraphicsBuffer = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRootConstantBufferView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
	else
	{
		m_pCommandList->SetGraphicsRootConstantBufferView(rootParamSlot, d3d12GraphicsBuffer->m_pResource->GetGPUVirtualAddress());
	}
}

void D3D12GraphicsContext::Set32BitConstants(unsigned int rootParamSlot, unsigned int num32BitValuesToSet, const void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline)
{
	BL_ASSERT(pSrcData);

	if (isComputePipeline)
	{
		m_pCommandList->SetComputeRoot32BitConstants(rootParamSlot, num32BitValuesToSet, pSrcData, offsetIn32BitValues);
	}
	else
	{
		m_pCommandList->SetGraphicsRoot32BitConstants(rootParamSlot, num32BitValuesToSet, pSrcData, offsetIn32BitValues);
	}
}

void D3D12GraphicsContext::SetIndexBuffer(IGraphicsBuffer* indexBuffer, unsigned int sizeInBytes)
{
	BL_ASSERT(indexBuffer);
	BL_ASSERT(sizeInBytes);

	D3D12GraphicsBuffer* d3d12IndexBuffer = static_cast<D3D12GraphicsBuffer*>(indexBuffer);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = d3d12IndexBuffer->m_pResource->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeInBytes;

	m_pCommandList->IASetIndexBuffer(&indexBufferView);
}

void D3D12GraphicsContext::DrawIndexedInstanced(unsigned int indexCountPerInstance, unsigned int instanceCount, unsigned int startIndexLocation, int baseVertexLocation, unsigned int startInstanceLocation)
{
	BL_ASSERT(indexCountPerInstance);
	BL_ASSERT(instanceCount);

	m_pCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
