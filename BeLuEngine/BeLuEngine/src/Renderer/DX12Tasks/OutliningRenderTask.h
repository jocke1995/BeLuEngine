#ifndef OUTLININGRENDERTASK_H
#define OUTLININGRENDERTASK_H

#include "RenderTask.h"
#include "../Renderer/Geometry/Transform.h"
class GraphicsState;

class OutliningRenderTask : public RenderTask
{
public:
	OutliningRenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~OutliningRenderTask();

	void Execute() override final;

	void SetObjectToOutline(std::pair<component::ModelComponent*, component::TransformComponent*>* objectToOutline);
	void Clear();
private:
	std::pair<component::ModelComponent*, component::TransformComponent*> m_ObjectToOutline;

	Transform m_OutlineTransformToScale = {};
};

#endif