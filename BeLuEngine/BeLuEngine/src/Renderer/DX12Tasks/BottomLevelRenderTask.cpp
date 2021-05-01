#include "stdafx.h"
#include "BottomLevelRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../Renderer/Geometry/Mesh.h"

#include "../DXR/BottomLevelAccelerationStructure.h"

BottomLevelRenderTask::BottomLevelRenderTask(
	ID3D12Device5* device,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DXRTask(device, FLAG_THREAD, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, clName)
{
}

BottomLevelRenderTask::~BottomLevelRenderTask()
{
}

void BottomLevelRenderTask::SubmitBLAS(BottomLevelAccelerationStructure* pBLAS)
{
	m_BLASesToUpdate.push_back(pBLAS);
}


void BottomLevelRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	for (BottomLevelAccelerationStructure* pBLAS : m_BLASesToUpdate)
	{
		pBLAS->BuildAccelerationStructure(commandList);
	}

	commandList->Close();

	// Flush list so that the bottom levels aren't updated again next frame.
	// If the behaviour of updating every Nth frame is needed, another Submit is required.
	m_BLASesToUpdate.clear();
}

