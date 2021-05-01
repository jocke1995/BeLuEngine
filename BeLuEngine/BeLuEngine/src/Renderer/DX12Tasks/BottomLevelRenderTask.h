#ifndef BOTTOMLEVELRENDERTASK_H
#define BOTTOMLEVELRENDERTASK_H

#include "DXRTask.h"

class BottomLevelAccelerationStructure;

class BottomLevelRenderTask : public DXRTask
{
public:
	BottomLevelRenderTask(
		ID3D12Device5* device,
		unsigned int FLAG_THREAD,
		const std::wstring& clName = L"DXRDefaultCommandListName");
	virtual ~BottomLevelRenderTask();
	
	void SubmitBLAS(BottomLevelAccelerationStructure* pBLAS);

	void Execute() override final;

private:
	std::vector<BottomLevelAccelerationStructure*> m_BLASesToUpdate;
};

#endif
