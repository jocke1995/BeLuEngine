#include "stdafx.h"
#include "ComputeTask.h"

// DX12 Specifics
#include "../PipelineState/ComputeState.h"

ComputeTask::ComputeTask(
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName,
	unsigned int FLAG_THREAD,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	const std::wstring& clName)
	:DX12Task(interfaceType, FLAG_THREAD, clName)
{
	for (auto& pair : csNamePSOName)
	{
		m_PipelineStates.push_back(new ComputeState(pair.first, pair.second));
	}
}

ComputeTask::~ComputeTask()
{
	for (auto cso : m_PipelineStates)
	{
		delete cso;
	}
	
}
