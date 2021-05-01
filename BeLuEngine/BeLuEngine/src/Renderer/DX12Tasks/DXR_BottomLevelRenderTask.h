#ifndef DXR_BOTTOMLEVELRENDERTASK_H
#define DXR_BOTTOMLEVELRENDERTASK_H

#include "DXRTask.h"

class DXR_BottomLevelRenderTask : public DXRTask
{
public:
	DXR_BottomLevelRenderTask(
		ID3D12Device5* device,
		unsigned int FLAG_THREAD,
		const std::wstring& clName = L"DXRDefaultCommandListName");
	virtual ~DXR_BottomLevelRenderTask();
	

	void Execute() override final;
private:

};

#endif
