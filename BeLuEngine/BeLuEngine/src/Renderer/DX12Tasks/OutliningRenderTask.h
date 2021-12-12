#ifndef OUTLININGRENDERTASK_H
#define OUTLININGRENDERTASK_H

#include "GraphicsPass.h"

#include "../Renderer/Geometry/Transform.h"

class BaseCamera;

class OutliningRenderTask : public GraphicsPass
{
public:
	OutliningRenderTask();
	~OutliningRenderTask();

	void Execute() override final;

	void SetObjectToOutline(std::pair<component::ModelComponent*, component::TransformComponent*>* objectToOutline);
	void Clear();
	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }
private:
	std::pair<component::ModelComponent*, component::TransformComponent*> m_ObjectToOutline;
	BaseCamera* m_pCamera = nullptr;

	Transform m_OutlineTransformToScale = {};
};

#endif