#ifndef DEPTHRENDERTASK_H
#define DEPTHRENDERTASK_H

#include "GraphicsPass.h"

class BaseCamera;
class IGraphicsContext;

class DepthRenderTask : public GraphicsPass
{
public:
	DepthRenderTask();
	~DepthRenderTask();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);
	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }

private:
	std::vector<RenderComponent> m_RenderComponents;
	BaseCamera* m_pCamera = nullptr;

	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		IGraphicsContext* graphicsContext);
};

#endif