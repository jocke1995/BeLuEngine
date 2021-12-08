#ifndef WIREFRAMERENDERTASK_H
#define WIREFRAMERENDERTASK_H

#include "RenderTask.h"

class GraphicsState;

#include "../../ECS/Components/BoundingBoxComponent.h"

class WireframeRenderTask : public RenderTask
{
public:
	WireframeRenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~WireframeRenderTask();

	void AddObjectToDraw(component::BoundingBoxComponent* bbc);

	void Clear();
	void ClearSpecific(component::BoundingBoxComponent* bbc);

	void Execute() override final;

private:
	std::vector<component::BoundingBoxComponent*> m_ObjectsToDraw;
};

#endif