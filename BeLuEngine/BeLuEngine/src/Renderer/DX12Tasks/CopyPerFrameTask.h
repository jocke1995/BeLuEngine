#ifndef COPYPERFRAMETASK_H
#define COPYPERFRAMETASK_H

#include "CopyTask.h"

class CopyPerFrameTask : public CopyTask
{
public:
	CopyPerFrameTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyPerFrameTask();

	void Execute();
};

#endif