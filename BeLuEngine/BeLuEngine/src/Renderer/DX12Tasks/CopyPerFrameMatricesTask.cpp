#include "stdafx.h"
#include "CopyPerFrameMatricesTask.h"

// DX12 Specifics
#include "../CommandInterface.h"

// For updates
#include "../Camera/BaseCamera.h"
#include "../Geometry/Transform.h"

TODO("Abstraction");
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"

CopyPerFrameMatricesTask::CopyPerFrameMatricesTask(
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:CopyTask(interfaceType, FLAG_THREAD, clName)
{

}

CopyPerFrameMatricesTask::~CopyPerFrameMatricesTask()
{

}

void CopyPerFrameMatricesTask::SubmitTransform(Transform* transform)
{
	m_TransformsToUpdate.push_back(transform);
}

void CopyPerFrameMatricesTask::SetCamera(BaseCamera* cam)
{
	m_pCameraRef = cam;
}

void CopyPerFrameMatricesTask::Execute()
{

	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	auto TransferResourceState = [commandList](IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		ID3D12Resource* resource = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer)->GetTempResource();
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		commandList->ResourceBarrier(1, &barrier);
	};

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(CopyPerFrameMatrices, commandList);
	
		for (Transform* transform: m_TransformsToUpdate)
		{
			DirectX::XMMATRIX w_wvp[2] = {};
	
			// World
			w_wvp[0] = *transform->GetWorldMatrixTransposed();
			// WVP
			w_wvp[1] = *m_pCameraRef->GetViewProjectionTranposed() * w_wvp[0];
	
			IGraphicsBuffer* graphicsBuffer = transform->GetConstantBuffer();
			ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer)->GetTempResource();

			D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
			ID3D12Device5* device5 = manager->GetDevice();

			unsigned __int64 sizeInBytes = graphicsBuffer->GetSize();
			DynamicDataParams dynamicDataParams = manager->SetDynamicData(sizeInBytes, &w_wvp);

			TransferResourceState(graphicsBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
			commandList->CopyBufferRegion(defaultHeap, 0, dynamicDataParams.uploadResource, dynamicDataParams.offsetFromStart, sizeInBytes);
			TransferResourceState(graphicsBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		}
	}
	commandList->Close();
}
