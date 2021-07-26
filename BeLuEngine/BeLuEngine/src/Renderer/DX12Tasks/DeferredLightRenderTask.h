#ifndef DEFERREDLIGHTASK_H
#define DEFERREDLIGHTASK_H

#include "RenderTask.h"

class ShaderResourceView;

class DeferredLightRenderTask : public RenderTask
{
public:
	DeferredLightRenderTask(ID3D12Device5* device, 
		ID3D12RootSignature* rootSignature,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~DeferredLightRenderTask();

	void Execute() override final;

	void SetSceneBVHSRV(ShaderResourceView* srv);

private:
	ShaderResourceView* m_pRayTracingSRV = nullptr;

	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl);
};

#endif