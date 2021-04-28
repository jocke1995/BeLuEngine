#include "stdafx.h"
#include "ComputeTask.h"

// DX12 Specifics
#include "../PipelineState/ComputeState.h"
#include "../RootSignature.h"

ComputeTask::ComputeTask(
	ID3D12Device5* device,
	ID3D12RootSignature* rootSignature,
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
	unsigned int FLAG_THREAD,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	const std::wstring& clName)
	:DX12Task(device, interfaceType, FLAG_THREAD, clName)
{
	for (auto& pair : csNamePSOName)
	{
		m_PipelineStates.push_back(new ComputeState(device, rootSignature, pair.first, pair.second));
	}

	m_pRootSig = rootSignature;
}

ComputeTask::~ComputeTask()
{
	for (auto cso : m_PipelineStates)
	{
		delete cso;
	}
	
}
