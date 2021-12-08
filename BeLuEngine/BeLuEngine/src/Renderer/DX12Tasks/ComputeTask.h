#ifndef COMPUTETASK_H
#define COMPUTETASK_H

#include "DX12Task.h"
#include "../CommandInterface.h"

class RootSignature;
class PipelineState;

class ComputeTask : public DX12Task
{
public:
	ComputeTask(
		std::vector<std::pair< std::wstring, std::wstring>> csNamePSOName,
		unsigned int FLAG_THREAD,
		E_COMMAND_INTERFACE_TYPE interfaceType = E_COMMAND_INTERFACE_TYPE::COMPUTE_TYPE,
		const std::wstring& clName = L"ComputeDefaultCommandListName");
	virtual ~ComputeTask();

protected:
	std::vector<PipelineState*> m_PipelineStates;
};
#endif
