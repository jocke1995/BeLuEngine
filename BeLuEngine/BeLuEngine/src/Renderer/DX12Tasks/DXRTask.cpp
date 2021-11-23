#include "stdafx.h"
#include "DXRTask.h"

// DX12 Specifics
#include "../PipelineState/ComputeState.h"


DXRTask::DXRTask(
	ID3D12Device5* device,
	unsigned int FLAG_THREAD,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	const std::wstring& clName)
	:DX12Task(device, interfaceType, FLAG_THREAD, clName)
{

}

DXRTask::~DXRTask()
{
}
