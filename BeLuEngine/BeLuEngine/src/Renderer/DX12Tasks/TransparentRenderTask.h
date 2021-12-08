#ifndef TRANSPARENTRENDERTASK_H
#define TRANSPARENTRENDERTASK_H

#include "RenderTask.h"

class TransparentRenderTask : public RenderTask
{
public:
	TransparentRenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~TransparentRenderTask();

	void Execute() override final;

private:
};

#endif