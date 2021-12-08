#include "stdafx.h"
#include "RenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../PipelineState/GraphicsState.h"

RenderTask::RenderTask(
	const std::wstring& VSName, const std::wstring& PSName,
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
	const std::wstring& psoName,
	unsigned int FLAG_THREAD)
	:DX12Task(E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, FLAG_THREAD, psoName)
{
	if (gpsds != nullptr)
	{
		for (auto gpsd : *gpsds)
		{
			m_PipelineStates.push_back(new GraphicsState(VSName, PSName, gpsd, psoName));
		}
	}
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

void RenderTask::SetRenderComponents(std::vector<RenderComponent> *renderComponents)
{
	m_RenderComponents = *renderComponents;
}

void RenderTask::SetCamera(BaseCamera* camera)
{
	m_pCamera = camera;
}
