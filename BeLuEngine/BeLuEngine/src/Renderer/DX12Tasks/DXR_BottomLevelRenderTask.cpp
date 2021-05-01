#include "stdafx.h"
#include "DXR_BottomLevelRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../Renderer/Geometry/Mesh.h"

DXR_BottomLevelRenderTask::DXR_BottomLevelRenderTask(
	ID3D12Device5* device,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DXRTask(device, FLAG_THREAD, E_COMMAND_INTERFACE_TYPE::COMPUTE_TYPE, clName)
{
}

DXR_BottomLevelRenderTask::~DXR_BottomLevelRenderTask()
{
}


void DXR_BottomLevelRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	

	commandList->Close();
}

