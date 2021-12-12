#ifndef WIREFRAMERENDERTASK_H
#define WIREFRAMERENDERTASK_H

#include "GraphicsPass.h"

class GraphicsState;
class BaseCamera;

#include "../../ECS/Components/BoundingBoxComponent.h"

class WireframeRenderTask : public GraphicsPass
{
public:
	WireframeRenderTask();
	~WireframeRenderTask();

	void Execute() override final;

	void AddObjectToDraw(component::BoundingBoxComponent* bbc);

	void Clear();
	void ClearSpecific(component::BoundingBoxComponent* bbc);

	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }

private:
	std::vector<component::BoundingBoxComponent*> m_ObjectsToDraw;
	BaseCamera* m_pCamera = nullptr;
};

#endif