#include "stdafx.h"
#include "BottomLevelRenderTask.h"

#include "../Renderer/API/IBottomLevelAS.h"
#include "../Renderer/API/IGraphicsContext.h"

BottomLevelRenderTask::BottomLevelRenderTask()
	:GraphicsPass(L"DXR_BottomlevelASPass")
{
}

BottomLevelRenderTask::~BottomLevelRenderTask()
{
}

void BottomLevelRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(Build_BLAS, m_pGraphicsContext);

		for (IBottomLevelAS* pBLAS : m_BLASesToUpdate)
		{
			m_pGraphicsContext->BuildBLAS(pBLAS);
		}
	}
	m_pGraphicsContext->End();

	// Flush list so that the bottom levels aren't updated again next frame.
	// If the behaviour of updating every Nth frame is needed, another Submit is required.
	m_BLASesToUpdate.clear();
}

void BottomLevelRenderTask::SubmitBLAS(IBottomLevelAS* pBLAS)
{
	m_BLASesToUpdate.push_back(pBLAS);
}
