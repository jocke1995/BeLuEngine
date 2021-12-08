#ifndef RENDERTASK_H
#define RENDERTASK_H

#include "Core.h"
#include <map>
#include "DX12Task.h"

// Renderer
class BaseCamera;
class PipelineState;

// Components
#include "../../ECS/Components/ModelComponent.h"
#include "../../ECS/Components/TransformComponent.h"
#include "../../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../../ECS/Components/Lights/PointLightComponent.h"
#include "../../ECS/Components/Lights/SpotLightComponent.h"

// DX12 Forward Declarations
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC;

class RenderTask : public DX12Task
{
public:
	RenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> *gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	
	virtual ~RenderTask();

	PipelineState* GetPipelineState(unsigned int index);

	void SetRenderComponents(std::vector<RenderComponent>* renderComponents);

	void SetCamera(BaseCamera* camera);
	
protected:
	std::vector<RenderComponent> m_RenderComponents;
	BaseCamera* m_pCamera = nullptr;
	std::vector<PipelineState*> m_PipelineStates;
};
#endif
