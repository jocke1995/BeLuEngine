#ifndef TONEMAPCOMPUTETASK_H
#define TONEMAPCOMPUTETASK_H

#include "../GraphicsPass.h"

class TonemapComputeTask : public GraphicsPass
{
public:
	TonemapComputeTask(unsigned int screenWidth, unsigned int screenHeight);
	virtual ~TonemapComputeTask();

	void Execute() override final;
private:
	unsigned int m_ScreenWidth = 0;
	unsigned int m_ScreenHeight = 0;
};

#endif
