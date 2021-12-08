#ifndef IMGUIRENDERTASK_H
#define IMGUIRENDERTASK_H

#include "RenderTask.h"
class ImGuiRenderTask : public RenderTask
{
public:
	ImGuiRenderTask(
		LPCWSTR VSName, LPCWSTR PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		LPCTSTR psoName,
		unsigned int FLAG_THREAD);
	virtual ~ImGuiRenderTask();

	void Execute();

private:
	
};

#endif

