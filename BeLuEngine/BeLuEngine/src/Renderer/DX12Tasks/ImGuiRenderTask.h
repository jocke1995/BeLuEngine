#ifndef IMGUIRENDERTASK_H
#define IMGUIRENDERTASK_H

#include "GraphicsPass.h"
class ImGuiRenderTask : public GraphicsPass
{
public:
	ImGuiRenderTask();
	virtual ~ImGuiRenderTask();

	void Execute();

private:
	
};

#endif

