#ifndef COPYTASK_H
#define COPYTASK_H

#include "DX12Task.h"

struct ID3D12GraphicsCommandList;

class CopyTask : public DX12Task
{
public:
	CopyTask(
		E_COMMAND_INTERFACE_TYPE interfaceType,
		unsigned int FLAG_THREAD,
		const std::wstring& clName);
	virtual ~CopyTask();

	virtual void Execute() = 0;
protected:
	
};
#endif