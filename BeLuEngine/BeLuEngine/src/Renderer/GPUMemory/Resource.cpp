#include "stdafx.h"
#include "Resource.h"

#include "../Misc/Log.h"

Resource::Resource(
	ID3D12Device* device,
	unsigned long long entrySize,
	RESOURCE_TYPE type,
	std::wstring name,
	D3D12_RESOURCE_FLAGS flags)
{
	m_Id = s_IdCounter++;

	m_EntrySize = entrySize;
	m_Type = type;
	m_Name = name;

	D3D12_HEAP_TYPE d3d12HeapType;
	switch (type)
	{
	case RESOURCE_TYPE::UPLOAD:
		d3d12HeapType = D3D12_HEAP_TYPE_UPLOAD;
		m_CurrResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;
	case RESOURCE_TYPE::DEFAULT:
		d3d12HeapType = D3D12_HEAP_TYPE_DEFAULT;
		m_CurrResourceState = D3D12_RESOURCE_STATE_COMMON;
		break;
	}

	setupHeapProperties(d3d12HeapType);

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = m_EntrySize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = flags;

	createResource(device, &resourceDesc, nullptr, m_CurrResourceState);
}

Resource::Resource(
	ID3D12Device* device,
	D3D12_RESOURCE_DESC* resourceDesc,
	D3D12_CLEAR_VALUE* clearValue,
	std::wstring name,
	D3D12_RESOURCE_STATES startState)
{
	m_Id = s_IdCounter++;
	m_Type = RESOURCE_TYPE::DEFAULT;
	m_EntrySize = resourceDesc->Width * resourceDesc->Height;
	m_Name = name;
	m_CurrResourceState = startState;
	D3D12_HEAP_TYPE d3d12HeapType = D3D12_HEAP_TYPE_DEFAULT;

	setupHeapProperties(d3d12HeapType);

	createResource(device, resourceDesc, clearValue, startState);
}

Resource::Resource()
{
	m_Id = s_IdCounter++;
}

bool Resource::operator==(const Resource& other)
{
	return m_Id == other.m_Id;
}

bool Resource::operator!=(const Resource& other)
{
	return !(operator==(other));
}

Resource::~Resource()
{
	SAFE_RELEASE(&m_pResource);
}

unsigned int Resource::GetSize() const
{
	return m_EntrySize;
}

ID3D12Resource1* Resource::GetID3D12Resource1() const
{
	return m_pResource;
}

ID3D12Resource1** Resource::GetID3D12Resource1PP()
{
	return &m_pResource;
}

D3D12_GPU_VIRTUAL_ADDRESS Resource::GetGPUVirtualAdress() const
{
	return m_pResource->GetGPUVirtualAddress();
}

void Resource::TransResourceState(
	ID3D12GraphicsCommandList5* cl,
	D3D12_RESOURCE_STATES stateBefore,
	D3D12_RESOURCE_STATES stateAfter)
{
	// TODO: only do this check in debug or debug_release
	if (SINGLE_THREADED_RENDERER == true)
	{
		if (m_CurrResourceState != stateBefore)
		{
			BL_LOG_WARNING("Resource barrier missmatch!\n");
		}
		m_CurrResourceState = stateAfter;
	}

	cl->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		m_pResource,
		stateBefore,
		stateAfter));
}

D3D12_RESOURCE_STATES Resource::GetCurrentState() const
{
	if (SINGLE_THREADED_RENDERER == true)
	{
		BL_LOG_WARNING("Resource::GetCurrentState() is not trustworthy when multithreading\n");
	}
	return m_CurrResourceState;
}

void Resource::SetData(const void* data, unsigned int subResourceIndex) const
{
	if (m_Type == RESOURCE_TYPE::DEFAULT)
	{
		BL_LOG_WARNING("Trying to Map into default heap\n");
		return;
	}

	void* dataBegin = nullptr;

	// Set up the heap data
	m_pResource->Map(subResourceIndex, nullptr, &dataBegin); // Get a dataBegin pointer where we can copy data to
	memcpy(dataBegin, data, m_EntrySize);
	m_pResource->Unmap(subResourceIndex, nullptr);
}

void Resource::setupHeapProperties(D3D12_HEAP_TYPE heapType)
{
	m_HeapProperties.Type = heapType;
	m_HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	m_HeapProperties.CreationNodeMask = 1; //used when multi-gpu
	m_HeapProperties.VisibleNodeMask = 1; //used when multi-gpu
	m_HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
}

void Resource::createResource(
	ID3D12Device* device,
	D3D12_RESOURCE_DESC* resourceDesc,
	D3D12_CLEAR_VALUE* clearValue,
	D3D12_RESOURCE_STATES startState)
{
	HRESULT hr = device->CreateCommittedResource(
		&m_HeapProperties,
		D3D12_HEAP_FLAG_NONE,
		resourceDesc,
		startState,
		clearValue,
		IID_PPV_ARGS(&m_pResource)
	);

	if (FAILED(hr))
	{
		std::string cbName(m_Name.begin(), m_Name.end());
		BL_LOG_CRITICAL("Failed to create Resource with name: \'%s\'\n", cbName.c_str());
	}

	m_pResource->SetName(m_Name.c_str());
}
