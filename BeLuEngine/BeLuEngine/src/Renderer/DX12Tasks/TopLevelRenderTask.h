#ifndef TOPLEVELRENDERTASK_H
#define TOPLEVELRENDERTASK_H

#include "GraphicsPass.h"

class TopLevelAccelerationStructure;

class TopLevelRenderTask : public GraphicsPass
{
public:
	TopLevelRenderTask();
	virtual ~TopLevelRenderTask();

	void Execute() override final;

	TopLevelAccelerationStructure* GetTLAS() const;

private:
	TopLevelAccelerationStructure* m_pTLAS = nullptr;
};

#endif
