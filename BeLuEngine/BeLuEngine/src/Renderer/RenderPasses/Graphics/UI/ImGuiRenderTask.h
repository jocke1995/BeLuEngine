#ifndef IMGUIRENDERTASK_H
#define IMGUIRENDERTASK_H

#include "../GraphicsPass.h"
class ImGuiRenderTask : public GraphicsPass
{
public:
	ImGuiRenderTask(unsigned int screenWidth, unsigned int screenHeight);
	virtual ~ImGuiRenderTask();

	void Execute();

private:
	
};

#endif

