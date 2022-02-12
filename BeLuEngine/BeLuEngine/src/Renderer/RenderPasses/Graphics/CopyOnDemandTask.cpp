#include "stdafx.h"
#include "CopyOnDemandTask.h"

#include "../Renderer/API/Interface/IGraphicsManager.h"
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsBuffer.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"

CopyOnDemandTask::CopyOnDemandTask()
	:GraphicsPass(L"CopyOnDemandPass")
{

}

CopyOnDemandTask::~CopyOnDemandTask()
{
}

TODO("Re-visit the entire lazy-copy situation that's going on here...")
void CopyOnDemandTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(CopyOnDemand, m_pGraphicsContext);

		//  Upload Buffers
		{
			ScopedPixEvent(Buffers, m_pGraphicsContext);
			for (std::pair<IGraphicsBuffer*, const void*> bufData : m_GraphicBuffersToUpload)
			{
				m_pGraphicsContext->ResourceBarrier(bufData.first, D3D12_RESOURCE_STATE_COPY_DEST);
				m_pGraphicsContext->UploadBuffer(bufData.first, bufData.second);
				m_pGraphicsContext->ResourceBarrier(bufData.first, D3D12_RESOURCE_STATE_COMMON);
			}
		}
		
		// Upload Textures
		{
			ScopedPixEvent(Textures, m_pGraphicsContext);
			for (IGraphicsTexture* graphicsTexture : m_GraphicTexturesToUpload)
			{
				m_pGraphicsContext->ResourceBarrier(graphicsTexture, D3D12_RESOURCE_STATE_COPY_DEST);
				m_pGraphicsContext->UploadTexture(graphicsTexture);
				m_pGraphicsContext->ResourceBarrier(graphicsTexture, D3D12_RESOURCE_STATE_COMMON);
			}
		}
	}
	m_pGraphicsContext->End();

	// Reset the buffers and textures, re-submit next frame if you want to update
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
