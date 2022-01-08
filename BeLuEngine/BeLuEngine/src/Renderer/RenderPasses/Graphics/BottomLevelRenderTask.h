#ifndef BOTTOMLEVELRENDERTASK_H
#define BOTTOMLEVELRENDERTASK_H

#include "GraphicsPass.h"

class IBottomLevelAS;

class BottomLevelRenderTask : public GraphicsPass
{
public:
	BottomLevelRenderTask();
	virtual ~BottomLevelRenderTask();
	
	void Execute() override final;

	void SubmitBLAS(IBottomLevelAS* pBLAS);

private:
	std::vector<IBottomLevelAS*> m_BLASesToUpdate;
};

#endif
