#ifndef TRANSPARENTRENDERTASK_H
#define TRANSPARENTRENDERTASK_H

#include "GraphicsPass.h"

class TransparentRenderTask : public GraphicsPass
{
public:
	TransparentRenderTask();
	~TransparentRenderTask();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);
private:
	std::vector<RenderComponent> m_RenderComponents;
};

#endif