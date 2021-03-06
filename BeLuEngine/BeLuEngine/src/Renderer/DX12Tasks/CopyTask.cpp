#include "stdafx.h"
#include "CopyTask.h"

// DX12 Specifics
#include "../CommandInterface.h"
#include "../GPUMemory/GPUMemory.h"

CopyTask::CopyTask(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:DX12Task(device, interfaceType, FLAG_THREAD, clName)
{

}

CopyTask::~CopyTask()
{

}

void CopyTask::Submit(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	m_UploadDefaultData.push_back(*Upload_Default_Data);
}

void CopyTask::copyResource(
	ID3D12GraphicsCommandList5* commandList,
	Resource* uploadResource, Resource* defaultResource,
	const void* data)
{
	// Set the data into the upload heap
	uploadResource->SetData(data);

	defaultResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST);

	// To Defaultheap from Uploadheap
	commandList->CopyResource(
		defaultResource->GetID3D12Resource1(),	// Receiever
		uploadResource->GetID3D12Resource1());	// Sender

	defaultResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON);
}