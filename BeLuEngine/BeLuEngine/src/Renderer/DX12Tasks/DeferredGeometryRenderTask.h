#ifndef DEFERREDGEOMETRYRENDERTASK_H
#define DEFERREDGEOMETRYRENDERTASK_H

#include "GraphicsPass.h"

class BaseCamera;

class DeferredGeometryRenderTask : public GraphicsPass
{
public:
	DeferredGeometryRenderTask();
	~DeferredGeometryRenderTask();

	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);
	void SetCamera(BaseCamera* baseCamera) { m_pCamera = baseCamera; }
private:
	std::vector<RenderComponent> m_RenderComponents;
	BaseCamera* m_pCamera = nullptr;

	void drawRenderComponent(
		component::ModelComponent* mc,
		component::TransformComponent* tc,
		const DirectX::XMMATRIX* viewProjTransposed,
		ID3D12GraphicsCommandList5* cl);
};

#endif