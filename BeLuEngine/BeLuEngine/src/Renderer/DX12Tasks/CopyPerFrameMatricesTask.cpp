#include "stdafx.h"
#include "CopyPerFrameMatricesTask.h"

// DX12 Specifics
#include "../CommandInterface.h"

// For updates
#include "../Camera/BaseCamera.h"
#include "../Geometry/Transform.h"

CopyPerFrameMatricesTask::CopyPerFrameMatricesTask(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:CopyTask(device, interfaceType, FLAG_THREAD, clName)
{

}

CopyPerFrameMatricesTask::~CopyPerFrameMatricesTask()
{

}

void CopyPerFrameMatricesTask::SetCamera(BaseCamera* cam)
{
	m_pCameraRef = cam;
}

void CopyPerFrameMatricesTask::Clear()
{
	m_UploadDefaultData.clear();
}

void CopyPerFrameMatricesTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(CopyPerFrameMatrices, commandList);

		for (auto& tuple : m_UploadDefaultData)
		{
			// Beautiful code :)
			Transform* t = static_cast<Transform*>(const_cast<void*>(std::get<2>(tuple)));

			DirectX::XMMATRIX w_wvp[2] = {};

			// World
			w_wvp[0] = *t->GetWorldMatrixTransposed();
			// WVP
			w_wvp[1] = *m_pCameraRef->GetViewProjectionTranposed() * w_wvp[0];

			copyResource(
				commandList,
				std::get<0>(tuple),		// UploadHeap
				std::get<1>(tuple),		// DefaultHeap
				w_wvp);					// Data
		}
	}
	commandList->Close();
}
