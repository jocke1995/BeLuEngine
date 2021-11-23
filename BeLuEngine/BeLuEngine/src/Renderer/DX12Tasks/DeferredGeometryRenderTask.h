#ifndef DEFERREDGEOMETRYRENDERTASK_H
#define DEFERREDGEOMETRYRENDERTASK_H

#include "RenderTask.h"

class DeferredGeometryRenderTask : public RenderTask
{
public:
	DeferredGeometryRenderTask(ID3D12Device5* device,
		ID3D12RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~DeferredGeometryRenderTask();

	void Execute() override final;

private:
	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl);
};

#endif