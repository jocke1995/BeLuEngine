#include "stdafx.h"
#include "DXRTask.h"

// DX12 Specifics
#include "../PipelineState/ComputeState.h"


DXRTask::DXRTask(unsigned int FLAG_THREAD, E_COMMAND_INTERFACE_TYPE interfaceType, const std::wstring& clName)
	:DX12Task(interfaceType, FLAG_THREAD, clName)
{

}

DXRTask::~DXRTask()
{
}
