#include "stdafx.h"
#include "CopyOnDemandTask.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"

// Stuff to copy
#include "../Renderer/Geometry/Mesh.h"

#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsBuffer.h"

CopyOnDemandTask::CopyOnDemandTask(
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:CopyTask(interfaceType, FLAG_THREAD, clName)
{

}

CopyOnDemandTask::~CopyOnDemandTask()
{
}

void CopyOnDemandTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(CopyOnDemand, commandList);

		//  Upload Buffers
		{
			ScopedPixEvent(Buffers, commandList);
			for (auto& buffer_data : m_GraphicBuffersToUpload)
			{
				this->CopyBuffer(commandList, &buffer_data);
			}
		}
		
		// Upload Textures
		{
			ScopedPixEvent(Textures, commandList);
			for (IGraphicsTexture* graphicsTexture : m_GraphicTexturesToUpload)
			{
				this->CopyTexture(commandList, graphicsTexture);
			}
		}
	}
	commandList->Close();

	// Reset the buffers and textures, resubmit next frame if you want to update
	this->Clear();
}

void CopyOnDemandTask::SubmitBuffer(IGraphicsBuffer* graphicsBuffer, const void* data)
{
	m_GraphicBuffersToUpload.push_back(std::make_pair(graphicsBuffer, data));
}

void CopyOnDemandTask::SubmitTexture(IGraphicsTexture* graphicsTexture)
{
	m_GraphicTexturesToUpload.push_back(graphicsTexture);
}

void CopyOnDemandTask::Clear()
{
	m_GraphicBuffersToUpload.clear();
	m_GraphicTexturesToUpload.clear();
}

void CopyOnDemandTask::CopyTexture(ID3D12GraphicsCommandList* cl, IGraphicsTexture* graphicsTexture)
{
	ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempResource();
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
	uploadResourceDesc.Width = graphicsTexture->GetSize();
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
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadHeap)
	);

	std::wstring resourceName = graphicsTexture->GetPath() + L"_UPLOAD_RESOURCE";

	if (!manager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to create a temporary uploadResource when copying data to VRAM.\nResourceName:\'%S\'\n", resourceName.c_str());
	}

	hr = uploadHeap->SetName(resourceName.c_str());
	if (!manager->SucceededHRESULT(hr))
	{
		BL_LOG_CRITICAL("Failed to Setname on UploadResource when uploading a Texture with name: \'%S\'\n", graphicsTexture->GetPath());
	}

	// Delete in NUM_SWAP_BUFFERS- frames from this point
	manager->AddD3D12ObjectToDefferedDeletion(uploadHeap);


#pragma endregion

	TransferResourceState(graphicsTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	// Transfer the data
	UpdateSubresources(cl,
		defaultHeap, uploadHeap,
		0, 0, static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempSubresources()->size(),
		static_cast<D3D12GraphicsTexture*>(graphicsTexture)->GetTempSubresources()->data());

	TransferResourceState(graphicsTexture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
}

void CopyOnDemandTask::CopyBuffer(ID3D12GraphicsCommandList* cl, std::pair<IGraphicsBuffer*, const void*>* graphicsBuffer_data)
{
	IGraphicsBuffer* graphicsBuffer = graphicsBuffer_data->first;
	ID3D12Resource* defaultHeap = static_cast<D3D12GraphicsBuffer*>(graphicsBuffer)->GetTempResource();

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

	unsigned __int64 sizeInBytes = graphicsBuffer->GetSize();
	DynamicDataParams dynamicDataParams = manager->SetDynamicData(sizeInBytes, graphicsBuffer_data->second);

	TransferResourceState(graphicsBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	cl->CopyBufferRegion(defaultHeap, 0, dynamicDataParams.uploadResource, dynamicDataParams.offsetFromStart, sizeInBytes);
	TransferResourceState(graphicsBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
}
