#ifndef DXRTASK_H
#define DXRTASK_H

#include "DX12Task.h"
#include "../CommandInterface.h"

class DXRTask : public DX12Task
{
public:
	DXRTask(
		ID3D12Device5* device,
		unsigned int FLAG_THREAD,
		E_COMMAND_INTERFACE_TYPE interfaceType = E_COMMAND_INTERFACE_TYPE::COMPUTE_TYPE,
		const std::wstring& clName = L"DXRDefaultCommandListName");

	virtual ~DXRTask();

private:

};
#endif
