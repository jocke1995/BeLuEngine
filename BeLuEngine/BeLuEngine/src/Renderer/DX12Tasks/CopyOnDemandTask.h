#ifndef COPYONDEMANDTASK_H
#define COPYONDEMANDTASK_H

#include "CopyTask.h"

class CopyOnDemandTask : public CopyTask
{
public:
	CopyOnDemandTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyOnDemandTask();

	void Execute() override final;

private:
};

#endif
