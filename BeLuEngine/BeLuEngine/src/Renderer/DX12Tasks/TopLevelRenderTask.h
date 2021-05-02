#ifndef TOPLEVELRENDERTASK_H
#define TOPLEVELRENDERTASK_H

#include "DXRTask.h"

class TopLevelAccelerationStructure;

class TopLevelRenderTask : public DXRTask
{
public:
	TopLevelRenderTask(
		ID3D12Device5* device,
		unsigned int FLAG_THREAD,
		const std::wstring& clName = L"DXRDefaultCommandListName");
	virtual ~TopLevelRenderTask();

	void Execute() override final;

	TopLevelAccelerationStructure* GetTLAS() const;

private:
	TopLevelAccelerationStructure* m_pTLAS = nullptr;
};

#endif
