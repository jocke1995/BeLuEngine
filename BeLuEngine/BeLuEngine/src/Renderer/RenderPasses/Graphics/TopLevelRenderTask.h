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

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);

	ITopLevelAS* GetTopLevelAS() const;
private:
	std::vector<RenderComponent> m_RenderComponents;

	ITopLevelAS* m_pTLAS = nullptr;
};

#endif
