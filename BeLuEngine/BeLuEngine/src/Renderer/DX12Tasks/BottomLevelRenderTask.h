#ifndef BOTTOMLEVELRENDERTASK_H
#define BOTTOMLEVELRENDERTASK_H

#include "GraphicsPass.h"

class BottomLevelAccelerationStructure;

class BottomLevelRenderTask : public GraphicsPass
{
public:
	BottomLevelRenderTask();
	virtual ~BottomLevelRenderTask();
	
	void SubmitBLAS(BottomLevelAccelerationStructure* pBLAS);

	void Execute() override final;

private:
	std::vector<BottomLevelAccelerationStructure*> m_BLASesToUpdate;
};

#endif
