#include "stdafx.h"
#include "TopLevelRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../GPUMemory/GPUMemory.h"
#include "../Renderer/Geometry/Mesh.h"

#include "../DXR/TopLevelAccelerationStructure.h"

TopLevelRenderTask::TopLevelRenderTask(
	ID3D12Device5* device,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DXRTask(device, FLAG_THREAD, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, clName)
{
	m_pTLAS = new TopLevelAccelerationStructure();
}

TopLevelRenderTask::~TopLevelRenderTask()
{
	SAFE_DELETE(m_pTLAS);
}

void TopLevelRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);

	static bool a = false;

	//if(!a)
		m_pTLAS->BuildAccelerationStructure(commandList);

	commandList->Close();

	a = true;
	m_pTLAS->m_IsBuilt = true;
}

TopLevelAccelerationStructure* TopLevelRenderTask::GetTLAS() const
{
	return m_pTLAS;
}

