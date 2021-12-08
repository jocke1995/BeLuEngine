#include "stdafx.h"
#include "CopyTask.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"

CopyTask::CopyTask(
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DX12Task(interfaceType, FLAG_THREAD, clName)
{

}

CopyTask::~CopyTask()
{

}