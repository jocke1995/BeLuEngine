#ifndef BOTTOMLEVELRENDERTASK_H
#define BOTTOMLEVELRENDERTASK_H

#include "GraphicsPass.h"

class BottomLevelAccelerationStructure;

class BottomLevelRenderTask : public GraphicsPass
{
public:
	BottomLevelRenderTask();
	virtual ~BottomLevelRenderTask();
	
	void Execute() override final;

	void SubmitBLAS(BottomLevelAccelerationStructure* pBLAS);

private:
	std::vector<BottomLevelAccelerationStructure*> m_BLASesToUpdate;
};

#endif
