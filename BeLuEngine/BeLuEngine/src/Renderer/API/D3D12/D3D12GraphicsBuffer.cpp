#include "stdafx.h"
#include "D3D12GraphicsBuffer.h"

#include "D3D12GraphicsManager.h"

#include "../Renderer/DescriptorHeap.h"

D3D12GraphicsBuffer::D3D12GraphicsBuffer(E_GRAPHICSBUFFER_TYPE type, unsigned int sizeOfSingleItem, unsigned int numItems, DXGI_FORMAT format, std::wstring name)
	:IGraphicsBuffer()
{
	BL_ASSERT_MESSAGE(sizeOfSingleItem, "Trying to create a buffer with a size of 0 bytes!\n");
	BL_ASSERT_MESSAGE(numItems, "Trying to create a buffer with a size of 0 bytes!\n");

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();
	DescriptorHeap* mainDHeap = graphicsManager->GetMainDescriptorHeap();

	unsigned int totalSizeUnpadded = sizeOfSingleItem * numItems;
	m_Size = totalSizeUnpadded;
	m_BufferType = type;

#pragma region SetParamsDependingOnType
	// Pad
	if(type == E_GRAPHICSBUFFER_TYPE::ConstantBuffer)
		m_Size = (totalSizeUnpadded + 255) & ~255;

	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES startState = D3D12_RESOURCE_STATE_COMMON;
	if (type == E_GRAPHICSBUFFER_TYPE::CPUBuffer)
	{
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		startState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	if (type == E_GRAPHICSBUFFER_TYPE::UnorderedAccessBuffer)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		startState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}

	if (type == E_GRAPHICSBUFFER_TYPE::RayTracingBuffer)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		startState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	}
#pragma endregion

#pragma region CreateBuffer
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
	resourceDesc.Flags = flags;
	resourceDesc.Format = format;

	HRESULT hr = graphicsManager->GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		startState,
		nullptr,
		IID_PPV_ARGS(&m_pResource)
	);

	if (!graphicsManager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create D3D12GraphicsBuffer with name: \'%S\'\n", name.c_str());
	}

	m_pResource->SetName(name.c_str());
#pragma endregion

#pragma region CreateDescriptor
	switch (type)
	{
		case E_GRAPHICSBUFFER_TYPE::ConstantBuffer:
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_pResource->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = m_Size;

			m_ConstantBufferDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
			device5->CreateConstantBufferView(&cbvDesc, mainDHeap->GetCPUHeapAt(m_ConstantBufferDescriptorHeapIndex));
			
			break;
		}
		case E_GRAPHICSBUFFER_TYPE::RawBuffer:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = numItems;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

			m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
			device5->CreateShaderResourceView(m_pResource, &srvDesc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex));

			break;
		}
		case E_GRAPHICSBUFFER_TYPE::VertexBuffer:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Format = format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = numItems;
			srvDesc.Buffer.StructureByteStride = sizeOfSingleItem;

			m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
			device5->CreateShaderResourceView(m_pResource, &srvDesc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex));

			break;
		}
		case E_GRAPHICSBUFFER_TYPE::IndexBuffer:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.FirstElement = 0;
			srvDesc.Format = format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Buffer.NumElements = numItems;
			srvDesc.Buffer.StructureByteStride = sizeOfSingleItem;

			m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
			device5->CreateShaderResourceView(m_pResource, &srvDesc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex));

			break;
		}
		case E_GRAPHICSBUFFER_TYPE::RayTracingBuffer:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			srvDesc.RaytracingAccelerationStructure.Location = m_pResource->GetGPUVirtualAddress();

			m_ShaderResourceDescriptorHeapIndex = mainDHeap->GetNextDescriptorHeapIndex(1);
			device5->CreateShaderResourceView(nullptr, &srvDesc, mainDHeap->GetCPUHeapAt(m_ShaderResourceDescriptorHeapIndex));

			break;
		}
	}
#pragma endregion
}

D3D12GraphicsBuffer::~D3D12GraphicsBuffer()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	BL_ASSERT(m_pResource);
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pResource);
}

unsigned int D3D12GraphicsBuffer::GetConstantBufferDescriptorIndex() const
{
	BL_ASSERT(m_BufferType == E_GRAPHICSBUFFER_TYPE::ConstantBuffer);
	return m_ConstantBufferDescriptorHeapIndex;
}

unsigned int D3D12GraphicsBuffer::GetShaderResourceHeapIndex() const
{
	BL_ASSERT(	m_BufferType == E_GRAPHICSBUFFER_TYPE::RawBuffer ||
				m_BufferType == E_GRAPHICSBUFFER_TYPE::VertexBuffer ||
				m_BufferType == E_GRAPHICSBUFFER_TYPE::IndexBuffer ||
				m_BufferType == E_GRAPHICSBUFFER_TYPE::RayTracingBuffer);
	return m_ShaderResourceDescriptorHeapIndex;
}

ID3D12Resource1* D3D12GraphicsBuffer::GetTempResource()
{
	BL_ASSERT(m_pResource);
	return m_pResource;;
}
