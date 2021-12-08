#include "stdafx.h"
#include "CopyOnDemandTask.h"

// DX12 Specifics
#include "../CommandInterface.h"

// Stuff to copy
#include "../Renderer/Geometry/Mesh.h"

CopyOnDemandTask::CopyOnDemandTask(
	ID3D12Device5* device,
	E_COMMAND_INTERFACE_TYPE interfaceType,
	unsigned int FLAG_THREAD,
	const std::wstring& clName)
	:CopyTask(device, interfaceType, FLAG_THREAD, clName)
{

}

CopyOnDemandTask::~CopyOnDemandTask()
{
}

void CopyOnDemandTask::Clear()
{
	TODO("Possible memory leak");
	//m_UploadDefaultData.clear();
	m_Textures.clear();
}


void CopyOnDemandTask::Execute()
{
	ID3D12CommandAllocator* commandAllocator = m_pCommandInterface->GetCommandAllocator(m_CommandInterfaceIndex);
	ID3D12GraphicsCommandList5* commandList = m_pCommandInterface->GetCommandList(m_CommandInterfaceIndex);

	m_pCommandInterface->Reset(m_CommandInterfaceIndex);
	{
		ScopedPixEvent(CopyOnDemand, commandList);

		// record the "small" data, such as constantbuffers..
		//for (auto& tuple : m_UploadDefaultData)
		//{
		//	copyResource(
		//		commandList,
		//		std::get<0>(tuple),		// UploadHeap
		//		std::get<1>(tuple),		// DefaultHeap
		//		std::get<2>(tuple));	// Data
		//}

		// record texturedata
		for (IGraphicsTexture* texture : m_Textures)
		{
			copyTexture(commandList, texture);
		}
	}
	commandList->Close();
}

void CopyOnDemandTask::copyTexture(ID3D12GraphicsCommandList5* commandList, IGraphicsTexture* texture)
{
	ID3D12Resource* defaultHeap = texture->m_pDefaultResource->GetID3D12Resource1();
	ID3D12Resource* uploadHeap  = texture->m_pUploadResource->GetID3D12Resource1();

	texture->m_pDefaultResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST);

	// Transfer the data
	UpdateSubresources(commandList,
		defaultHeap, uploadHeap,
		0, 0, static_cast<unsigned int>(texture->m_SubresourceData.size()),
		texture->m_SubresourceData.data());

	texture->m_pDefaultResource->TransResourceState(
		commandList,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON);
}
