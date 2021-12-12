#ifndef TRANSPARENTRENDERTASK_H
#define TRANSPARENTRENDERTASK_H

#include "GraphicsPass.h"

class BaseCamera;

class TransparentRenderTask : public GraphicsPass
{
public:
	TransparentRenderTask();
	~TransparentRenderTask();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);
	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }
private:
	std::vector<RenderComponent> m_RenderComponents;
	BaseCamera* m_pCamera = nullptr;
};

#endif