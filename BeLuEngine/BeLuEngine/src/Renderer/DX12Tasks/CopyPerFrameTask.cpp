#include "stdafx.h"
#include "CopyPerFrameTask.h"

// DX12 Specifics
#include "../CommandInterface.h"

CopyPerFrameTask::CopyPerFrameTask(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:CopyTask(device, interfaceType, FLAG_THREAD, clName)
{

}

CopyPerFrameTask::~CopyPerFrameTask()
{

}

void CopyPerFrameTask::Clear()
{
	m_UploadDefaultData.clear();
}

void CopyPerFrameTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(CopyPerFrame, commandList);

		for (auto& tuple : m_UploadDefaultData)
		{
			copyResource(
				commandList,
				std::get<0>(tuple),		// UploadHeap
				std::get<1>(tuple),		// DefaultHeap
				std::get<2>(tuple));	// Data
		}
	}
	commandList->Close();
}
