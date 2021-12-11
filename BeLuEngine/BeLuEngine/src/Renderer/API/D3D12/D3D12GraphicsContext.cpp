#include "stdafx.h"
#include "D3D12GraphicsContext.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "D3D12GraphicsManager.h"
#include "../Renderer/DescriptorHeap.h"	TODO("Remove");

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

void D3D12GraphicsContext::Begin(bool isComputePipeline)
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	unsigned int index = graphicsManager->GetCommandInterfaceIndex();

	// Reset commandinterface
	D3D12GraphicsManager::SucceededHRESULT(m_pCommandAllocators[index]->Reset());
	D3D12GraphicsManager::SucceededHRESULT(m_pCommandList->Reset(m_pCommandAllocators[index], NULL));

	// Rootsignature
	if (isComputePipeline)
		m_pCommandList->SetComputeRootSignature(graphicsManager->GetGlobalRootSignature());
	else
		m_pCommandList->SetGraphicsRootSignature(graphicsManager->GetGlobalRootSignature());

	// DescriptorHeap
	ID3D12DescriptorHeap* d3d12DescriptorHeap = graphicsManager->GetMainDescriptorHeap()->GetID3D12DescriptorHeap();
	m_pCommandList->SetDescriptorHeaps(1, &d3d12DescriptorHeap);

	// Set all DescriptorTables
	m_pCommandList->SetGraphicsRootDescriptorTable(dtSRV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_pCommandList->SetGraphicsRootDescriptorTable(dtCBV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	m_pCommandList->SetGraphicsRootDescriptorTable(dtUAV, d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void D3D12GraphicsContext::End()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	D3D12GraphicsManager::SucceededHRESULT(m_pCommandList->Close());
}

void D3D12GraphicsContext::SetPipelineState(ID3D12PipelineState* pso)
{
	m_pCommandList->SetPipelineState(pso);
}

void D3D12GraphicsContext::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop)
{
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

void D3D12GraphicsContext::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, float clearColor[4])
{
	m_pCommandList->ClearRenderTargetView(cpuHandle, clearColor, 0, nullptr);
}

void D3D12GraphicsContext::SetRenderTargets(unsigned int numRenderTargets, D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetDescriptors, bool RTsSingleHandleToDescriptorRange, D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilDescriptor)
{
	m_pCommandList->OMSetRenderTargets(numRenderTargets, renderTargetDescriptors, RTsSingleHandleToDescriptorRange, depthStencilDescriptor);
}


//void D3D12GraphicsContext::SetConstantBufferView(unsigned int slot, Resource* resource, bool isComputePipeline)
//{
//	if (isComputePipeline)
//		m_pCommandList->SetComputeRootConstantBufferView(slot, resource->GetGPUVirtualAdress());
//	else
//		m_pCommandList->SetGraphicsRootConstantBufferView(slot, resource->GetGPUVirtualAdress());
//}
//
//void D3D12GraphicsContext::SetShaderResourceView(unsigned int slot, Resource* resource, bool isComputePipeline)
//{
//	if (isComputePipeline)
//		m_pCommandList->SetComputeRootShaderResourceView(slot, resource->GetGPUVirtualAdress());
//	else
//		m_pCommandList->SetComputeRootShaderResourceView(slot, resource->GetGPUVirtualAdress());
//}

void D3D12GraphicsContext::Set32BitConstant(unsigned int slot, unsigned int num32BitValuesToSet, unsigned int* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline)
{
	if (isComputePipeline)
		m_pCommandList->SetComputeRoot32BitConstant(slot, *pSrcData, offsetIn32BitValues);
	else
		m_pCommandList->SetGraphicsRoot32BitConstant(slot, *pSrcData, offsetIn32BitValues);
}

void D3D12GraphicsContext::SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView)
{
	m_pCommandList->IASetIndexBuffer(indexBufferView);
}

void D3D12GraphicsContext::DrawIndexedInstanced(unsigned int indexCountPerInstance, unsigned int instanceCount, unsigned int startIndexLocation, int baseVertexLocation, unsigned int startInstanceLocation)
{
	m_pCommandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}
