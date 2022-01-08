#ifndef TOPLEVELRENDERTASK_H
#define TOPLEVELRENDERTASK_H

#include "GraphicsPass.h"

class ITopLevelAS;

class TopLevelRenderTask : public GraphicsPass
{
public:
	TopLevelRenderTask();
	virtual ~TopLevelRenderTask();

	void Execute() override final;

	ITopLevelAS* GetTLAS() const;

private:
	ITopLevelAS* m_pTLAS = nullptr;
};

#endif
