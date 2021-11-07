#include "stdafx.h"
#include "CommandInterface.h"

#include "../Misc/Log.h"

CommandInterface::CommandInterface(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	const std::wstring& clName)
{
	m_Name = clName;
	createCommandInterfaces(device, interfaceType);
}

CommandInterface::~CommandInterface()
{
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_RELEASE(&m_pCommandAllocators[i]);
		BL_SAFE_RELEASE(&m_pCommandLists[i]);
	}
}

ID3D12GraphicsCommandList5* CommandInterface::GetCommandList(unsigned int index) const
{
	return m_pCommandLists[index];
}

ID3D12CommandAllocator* CommandInterface::GetCommandAllocator(unsigned int index) const
{
	return m_pCommandAllocators[index];
}

void CommandInterface::Reset(unsigned int index)
{
	m_pCommandAllocators[index]->Reset();

	m_pCommandLists[index]->Reset(m_pCommandAllocators[index], NULL);
}

void CommandInterface::createCommandInterfaces(ID3D12Device5* device, E_COMMAND_INTERFACE_TYPE interfaceType)
{
	D3D12_COMMAND_LIST_TYPE D3D12type;
	switch (interfaceType)
	{
	case E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE:
		D3D12type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	case E_COMMAND_INTERFACE_TYPE::COPY_TYPE:
		D3D12type = D3D12_COMMAND_LIST_TYPE_COPY;
		break;
	case E_COMMAND_INTERFACE_TYPE::COMPUTE_TYPE:
		D3D12type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		break;
	default:
		D3D12type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		break;
	}

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		HRESULT hr = device->CreateCommandAllocator(D3D12type, IID_PPV_ARGS(&m_pCommandAllocators[i]));

		if (FAILED(hr))
		{
			BL_LOG_CRITICAL("Failed to Create CommandAllocator\n");
		}

		hr = device->CreateCommandList(0,
			D3D12type,
			m_pCommandAllocators[i],
			nullptr,
			IID_PPV_ARGS(&m_pCommandLists[i]));

		m_pCommandAllocators[i]->SetName((m_Name + L"_CmdList").c_str());
		m_pCommandLists[i]->SetName((m_Name + L"_CmdList").c_str());
		if (FAILED(hr))
		{
			BL_LOG_CRITICAL("Failed to Create CommandList\n");
		}

		m_pCommandLists[i]->Close();
	}
}
