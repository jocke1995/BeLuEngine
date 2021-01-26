#include "stdafx.h"
#include "RenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../GPUMemory/GPUMemory.h"
#include "../PipelineState/GraphicsState.h"
#include "../RootSignature.h"
#include "../SwapChain.h"



RenderTask::RenderTask(
	ID3D12Device5* device,
	RootSignature* rootSignature,
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:DX12Task(device, COMMAND_INTERFACE_TYPE::DIRECT_TYPE, FLAG_THREAD, psoName)
{
	if (gpsds != nullptr)
	{
		for (auto gpsd : *gpsds)
		{
			m_PipelineStates.push_back(new GraphicsState(device, rootSignature, VSName, PSName, gpsd, psoName));
		}
	}
	
	m_pRootSig = rootSignature->GetRootSig();
}

RenderTask::~RenderTask()
{
	for (auto pipelineState : m_PipelineStates)
		delete pipelineState;
}

PipelineState* RenderTask::GetPipelineState(unsigned int index)
{
	return m_PipelineStates[index];
}

void RenderTask::AddRenderTargetView(std::string name, const RenderTargetView* renderTargetView)
{
	m_RenderTargetViews[name] = renderTargetView;
}

void RenderTask::SetRenderComponents(std::vector<std::pair<	component::ModelComponent*,
															component::TransformComponent*>>*renderComponents)
{
	m_RenderComponents = *renderComponents;
}

void RenderTask::SetMainDepthStencil(DepthStencil* depthStencil)
{
	m_pDepthStencil = depthStencil;
}

void RenderTask::SetCamera(BaseCamera* camera)
{
	m_pCamera = camera;
}

void RenderTask::SetSwapChain(SwapChain* swapChain)
{
	m_pSwapChain = swapChain;
}



