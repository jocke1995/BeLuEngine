#ifndef RENDERTASK_H
#define RENDERTASK_H

#include "Core.h"
#include <map>
#include "DX12Task.h"

// Renderer
class RootSignature;
class Resource;

class DepthStencil;
class BaseCamera;
class RenderTargetView;
class SwapChain;
class PipelineState;

// Components
#include "../../ECS/Components/ModelComponent.h"
#include "../../ECS/Components/TransformComponent.h"
#include "../../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../../ECS/Components/Lights/PointLightComponent.h"
#include "../../ECS/Components/Lights/SpotLightComponent.h"

#include "../GPUMemory/ConstantBuffer.h"

// DX12 Forward Declarations
struct ID3D12RootSignature;
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC;

struct RenderComponent
{
public:
	RenderComponent(component::ModelComponent* mc, component::TransformComponent* tc)
		:mc(mc), tc(tc){};

	component::ModelComponent* mc = nullptr;
	component::TransformComponent* tc = nullptr;
};

class RenderTask : public DX12Task
{
public:
	RenderTask(ID3D12Device5* device, 
		RootSignature* rootSignature, 
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> *gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	
	virtual ~RenderTask();

	PipelineState* GetPipelineState(unsigned int index);

	
	void AddRenderTargetView(std::string, const RenderTargetView* renderTargetView);
	
	void SetRenderComponents(std::vector<RenderComponent>* renderComponents);
	void SetMainDepthStencil(DepthStencil* depthStencil);

	void SetCamera(BaseCamera* camera);
	void SetSwapChain(SwapChain* swapChain);
	
protected:
	std::vector<RenderComponent> m_RenderComponents;
	std::map<std::string, const RenderTargetView*> m_RenderTargetViews;
	
	DepthStencil* m_pDepthStencil = nullptr;
	BaseCamera* m_pCamera = nullptr;
	SwapChain* m_pSwapChain = nullptr;
	ID3D12RootSignature* m_pRootSig = nullptr;
	std::vector<PipelineState*> m_PipelineStates;
};
#endif
