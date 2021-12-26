#ifndef WIREFRAMERENDERTASK_H
#define WIREFRAMERENDERTASK_H

#include "GraphicsPass.h"

#include "../ECS/Components/BoundingBoxComponent.h"
class WireframeRenderTask : public GraphicsPass
{
public:
	WireframeRenderTask();
	~WireframeRenderTask();

	void Execute() override final;

	void AddObjectToDraw(component::BoundingBoxComponent* bbc);

	void Clear();
	void ClearSpecific(component::BoundingBoxComponent* bbc);

private:
	std::vector<component::BoundingBoxComponent*> m_ObjectsToDraw;
};

#endif