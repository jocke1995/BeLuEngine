#include "stdafx.h"
#include "DX12Task.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"


// PIX Events
#include "WinPixEventRuntime/pix3.h"
ScopedPIXEvent::ScopedPIXEvent(const char* nameOfTask, ID3D12GraphicsCommandList* cl)
{
	assert(cl);

	m_pCommandList = cl;
	UINT64 col = 0;
	PIXBeginEvent(m_pCommandList, col, nameOfTask);
}

ScopedPIXEvent::~ScopedPIXEvent()
{
	PIXEndEvent(m_pCommandList);
}

// DX12 TASK
DX12Task::DX12Task(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:MultiThreadedTask(FLAG_THREAD)
{
	m_pCommandInterface = new CommandInterface(device, interfaceType, clName);
}

DX12Task::~DX12Task()
{
	delete m_pCommandInterface;
}

void DX12Task::SetBackBufferIndex(int backBufferIndex)
{
	m_BackBufferIndex = backBufferIndex;
}

void DX12Task::SetCommandInterfaceIndex(int index)
{
	m_CommandInterfaceIndex = index;
}

void DX12Task::SetDescriptorHeaps(std::map<E_DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> dhs)
{
	m_DescriptorHeaps = dhs;
}

void DX12Task::AddResource(std::string id, const Resource* resource)
{
	if (m_Resources[id] == nullptr)
	{
		m_Resources[id] = resource;
	}
	else
	{
		BL_LOG_CRITICAL("Trying to add Resource with name: \'%s\' that already exists.\n", id);
	}
}

CommandInterface* const DX12Task::GetCommandInterface() const
{
	return m_pCommandInterface;
}
