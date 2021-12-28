#include "stdafx.h"
#include "D3D12DescriptorHeap.h"

#include "D3D12GraphicsManager.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numDescriptors, std::wstring name, unsigned int GPUVisible)
{
	ID3D12Device5* device = D3D12GraphicsManager::GetInstance()->GetDevice();

	// Create description
	D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
	dhDesc.Type = type;
	dhDesc.NumDescriptors = numDescriptors;
	dhDesc.Flags = GPUVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	m_HandleIncrementSize = device->GetDescriptorHandleIncrementSize(dhDesc.Type);

	HRESULT hr = device->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_pDescriptorHeap));
	if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create DescriptorHeap: %S\n", name);
	}

	m_pDescriptorHeap->SetName(name.c_str());

	// Set start
	m_CPUHeapStart = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (GPUVisible)
		m_GPUHeapStart = m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
	BL_SAFE_RELEASE(&m_pDescriptorHeap);
}

void D3D12DescriptorHeap::IncrementDescriptorHeapIndex()
{
	m_DescriptorHeapIndex++;
}

unsigned int D3D12DescriptorHeap::GetNextDescriptorHeapIndex(unsigned int increment)
{
	unsigned int indexToReturn = m_DescriptorHeapIndex;

	m_DescriptorHeapIndex += increment;

	return indexToReturn;
}

ID3D12DescriptorHeap* D3D12DescriptorHeap::GetID3D12DescriptorHeap() const
{
	return m_pDescriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetCPUHeapAt(unsigned int descriptorIndex)
{
	m_CPUHeapAt.ptr = m_CPUHeapStart.ptr + m_HandleIncrementSize * descriptorIndex;
	return m_CPUHeapAt;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GetGPUHeapAt(unsigned int descriptorIndex)
{
	m_GPUHeapAt.ptr = m_GPUHeapStart.ptr + m_HandleIncrementSize * descriptorIndex;

	return m_GPUHeapAt;
}
