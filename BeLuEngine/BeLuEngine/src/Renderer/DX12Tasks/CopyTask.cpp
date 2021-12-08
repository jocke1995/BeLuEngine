#include "stdafx.h"
#include "CopyTask.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"

#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"

CopyTask::CopyTask(
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DX12Task(interfaceType, FLAG_THREAD, clName)
{

}

CopyTask::~CopyTask()
{

}

void CopyTask::SubmitBuffer(IGraphicsBuffer* graphicsBuffer, const void* data)
{
	GraphicsBufferUploadParams param = {graphicsBuffer, data};
	m_GraphicBuffersToUpload.push_back(param);
}

void CopyTask::SubmitTexture(IGraphicsTexture* graphicsTexture, const void* data)
{
	GraphicsTextureUploadParams param = { graphicsTexture, data };
	m_GraphicTexturesToUpload.push_back(param);
}

void CopyTask::Clear()
{
	m_GraphicBuffersToUpload.clear();
	m_GraphicTexturesToUpload.clear();
}

void CopyTask::CopyTexture(ID3D12GraphicsCommandList* cl, GraphicsTextureUploadParams* uploadParams)
{
	ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsTexture*>(uploadParams->graphicsTexture)->GetTempResource();
	ID3D12Resource* uploadHeap = nullptr;

	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	auto TransferResourceState = [cl](IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		ID3D12Resource* resource = static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempResource();
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;
		
		cl->ResourceBarrier(1, &barrier);
	};

#pragma region CreateImediateUploadResource

	HRESULT hr;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1; //used when multi-gpu
	heapProps.VisibleNodeMask = 1; //used when multi-gpu
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC uploadResourceDesc = {};
	uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadResourceDesc.Width = uploadParams->graphicsTexture->GetSize();
	uploadResourceDesc.Height = 1;
	uploadResourceDesc.DepthOrArraySize = 1;
	uploadResourceDesc.MipLevels = 1;
	uploadResourceDesc.SampleDesc.Count = 1;
	uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	hr = device5->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&uploadResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&uploadHeap)
	);

	std::wstring resourceName = uploadParams->graphicsTexture->GetPath() + L"_UPLOAD_RESOURCE";

	if (!manager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create D3D12GraphicsTexture with name: \'%s\'\n", resourceName.c_str());
	}

	hr = uploadHeap->SetName(resourceName.c_str());
	if (!manager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to Setname on UploadResource when uploading a Texture with name: \'%s\'\n", uploadParams->graphicsTexture->GetPath());
	}

	// Delete in NUM_SWAP_BUFFERS- frames from this point
	manager->AddD3D12ObjectToDefferedDeletion(uploadHeap);

	
#pragma endregion

	TransferResourceState(uploadParams->graphicsTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	D3D12_SUBRESOURCE_DATA data = {};
	data.pData = uploadParams->data;

	// Transfer the data
	UpdateSubresources(cl,
		defaultHeap, uploadHeap,
		0, 0, static_cast<D3D12GraphicsTexture*>(uploadParams->graphicsTexture)->GetTempSubresources()->size(),
		static_cast<D3D12GraphicsTexture*>(uploadParams->graphicsTexture)->GetTempSubresources()->data());

	TransferResourceState(uploadParams->graphicsTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
}

void CopyTask::CopyBuffer(ID3D12GraphicsCommandList* cl, GraphicsBufferUploadParams* uploadParams)
{
	ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsBuffer*>(uploadParams->graphicsBuffer)->GetTempResource();

	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	auto TransferResourceState = [cl](IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		ID3D12Resource* resource = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer)->GetTempResource();
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		cl->ResourceBarrier(1, &barrier);
	};

	unsigned __int64 sizeInBytes = uploadParams->graphicsBuffer->GetSize();
	DynamicDataParams dynamicDataParams = manager->SetDynamicData(sizeInBytes, uploadParams->data);

	TransferResourceState(uploadParams->graphicsBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cl->CopyBufferRegion(defaultHeap, 0, dynamicDataParams.uploadResource, dynamicDataParams.offsetFromStart, sizeInBytes);
	TransferResourceState(uploadParams->graphicsBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
}

//void CopyTask::ClearSpecific(const Resource* uploadResource)
//{
//	unsigned int i = 0;
//
//	// Loop through all copyPerFrame tasks
//	for (auto& tuple : m_UploadDefaultData)
//	{
//		if (std::get<0>(tuple) == uploadResource)
//		{
//			// Remove
//			m_UploadDefaultData.erase(m_UploadDefaultData.begin() + i);
//		}
//		i++;
//	}
//}