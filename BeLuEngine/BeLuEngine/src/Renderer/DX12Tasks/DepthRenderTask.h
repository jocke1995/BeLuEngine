#ifndef DEPTHRENDERTASK_H
#define DEPTHRENDERTASK_H

#include "GraphicsPass.h"

class IGraphicsContext;

class DepthRenderTask : public GraphicsPass
{
public:
	DepthRenderTask();
	~DepthRenderTask();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);

private:
	std::vector<RenderComponent> m_RenderComponents;

	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		IGraphicsContext* graphicsContext);
};

#endif