#include "stdafx.h"
#include "D3D12GraphicsBuffer.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "D3D12GraphicsManager.h"

#include "../Renderer/DescriptorHeap.h"

D3D12GraphicsBuffer::D3D12GraphicsBuffer(E_GRAPHICSBUFFER_TYPE type, E_GRAPHICSBUFFER_UPLOADFREQUENCY uploadFrequency, unsigned int size, std::wstring name)
	:IGraphicsBuffer()
{
	BL_ASSERT_MESSAGE(size, "Trying to create a buffer with a size of 0 bytes!\n");

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	m_Size = (size + 255) & ~255;

#pragma region CreateBuffer
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = heapType;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1; //used when multi-gpu
	heapProps.VisibleNodeMask = 1; //used when multi-gpu
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = m_Size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = graphicsManager->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_pResource)
	);

	if (!graphicsManager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create D3D12GraphicsBuffer with name: \'%s\'\n", name.c_str());
	}

	m_pResource->SetName(name.c_str());
#pragma endregion

#pragma region CreateDescriptor
	switch (type)
	{
		case E_GRAPHICSBUFFER_TYPE::ConstantBuffer:
		{
			m_ConstantBufferSlot = mainDHeap->GetNextDescriptorHeapIndex(1);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_pResource->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = m_Size;

			device5->CreateConstantBufferView(&cbvDesc, mainDHeap->GetCPUHeapAt(m_ConstantBufferSlot));
			
			break;
		}
		case E_GRAPHICSBUFFER_TYPE::RawBuffer:
		{
			m_RawBufferSlot = mainDHeap->GetNextDescriptorHeapIndex(1);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = 1;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

			device5->CreateShaderResourceView(m_pResource, &srvDesc, mainDHeap->GetCPUHeapAt(m_RawBufferSlot));

			break;
		}
		default:
		{
			BL_LOG_CRITICAL("Invalid E_GRAPHICSBUFFER_TYPE when trying to create: %s", name);
			break;
		}
	}
#pragma endregion
}

D3D12GraphicsBuffer::~D3D12GraphicsBuffer()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pResource);
}

unsigned int D3D12GraphicsBuffer::GetConstantBufferDescriptorIndex() const
{
	BL_ASSERT(m_BufferType == E_GRAPHICSBUFFER_TYPE::ConstantBuffer);
	return m_ConstantBufferSlot;
}

unsigned int D3D12GraphicsBuffer::GetRawBufferDescriptorIndex() const
{
	BL_ASSERT(m_BufferType == E_GRAPHICSBUFFER_TYPE::RawBuffer);
	return m_RawBufferSlot;
}
