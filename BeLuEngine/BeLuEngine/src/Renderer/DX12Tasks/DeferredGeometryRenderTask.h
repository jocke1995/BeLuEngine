#ifndef DEFERREDGEOMETRYRENDERTASK_H
#define DEFERREDGEOMETRYRENDERTASK_H

#include "GraphicsPass.h"

class DeferredGeometryRenderTask : public GraphicsPass
{
public:
	DeferredGeometryRenderTask();
	~DeferredGeometryRenderTask();

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