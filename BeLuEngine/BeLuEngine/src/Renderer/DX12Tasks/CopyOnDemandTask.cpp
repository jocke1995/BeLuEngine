#include "stdafx.h"
#include "CopyOnDemandTask.h"

#include "../Misc/Log.h"

// DX12 Specifics
#include "../CommandInterface.h"

// Stuff to copy
#include "../Renderer/Geometry/Mesh.h"

#include "../Renderer/API/D3D12/D3D12GraphicsTexture.h"
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"

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
			for (GraphicsBufferUploadParams& bufferParam : m_GraphicBuffersToUpload)
			{
				this->CopyBuffer(commandList, &bufferParam);
			}
		}
		
		// Upload Textures
		{
			ScopedPixEvent(Textures, commandList);
			for (GraphicsTextureUploadParams& textureParam : m_GraphicTexturesToUpload)
			{
				this->CopyTexture(commandList, &textureParam);
			}
		}
	}
	commandList->Close();

	// Reset the buffers and textures, resubmit next frame if you want to update
	this->Clear();
}
