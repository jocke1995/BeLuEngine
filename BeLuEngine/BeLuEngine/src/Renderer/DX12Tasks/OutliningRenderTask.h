#ifndef OUTLININGRENDERTASK_H
#define OUTLININGRENDERTASK_H

#include "RenderTask.h"
#include "../Renderer/Geometry/Transform.h"
class GraphicsState;
class SwapChain;

class OutliningRenderTask : public RenderTask
{
public:
	OutliningRenderTask(ID3D12Device5* device,
		ID3D12RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		DescriptorHeap* cbvHeap,
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