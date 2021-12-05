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
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_RELEASE(&m_pCommandAllocators[i]);
	}
	BL_SAFE_RELEASE(&m_pCommandList);
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
