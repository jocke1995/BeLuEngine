#include "stdafx.h"
#include "TopLevelRenderTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../DescriptorHeap.h"
#include "../Renderer/Geometry/Mesh.h"

#include "../DXR/TopLevelAccelerationStructure.h"

TopLevelRenderTask::TopLevelRenderTask()
	:GraphicsPass(L"DXR_TopLevelASPass")
{
	m_pTLAS = new TopLevelAccelerationStructure();
}

TopLevelRenderTask::~TopLevelRenderTask()
{
	BL_SAFE_DELETE(m_pTLAS);
}

void TopLevelRenderTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(DXR_TLAS, commandList);

		m_pTLAS->BuildAccelerationStructure(commandList);

		m_pTLAS->m_IsBuilt = true;
	}
	commandList->Close();
}

TopLevelAccelerationStructure* TopLevelRenderTask::GetTLAS() const
{
	return m_pTLAS;
}

