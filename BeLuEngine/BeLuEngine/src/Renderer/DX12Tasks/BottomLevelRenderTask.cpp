#include "stdafx.h"
#include "BottomLevelRenderTask.h"

#include "../DXR/BottomLevelAccelerationStructure.h"

#include "../API/IGraphicsContext.h"

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

		for (BottomLevelAccelerationStructure* pBLAS : m_BLASesToUpdate)
		{
			m_pGraphicsContext->BuildAccelerationStructure(pBLAS->GetBuildDesc());
			m_pGraphicsContext->UAVBarrier(pBLAS->GetRayTracingResultBuffer());
		}
	}
	m_pGraphicsContext->End();

	// Flush list so that the bottom levels aren't updated again next frame.
	// If the behaviour of updating every Nth frame is needed, another Submit is required.
	m_BLASesToUpdate.clear();
}

void BottomLevelRenderTask::SubmitBLAS(BottomLevelAccelerationStructure* pBLAS)
{
	m_BLASesToUpdate.push_back(pBLAS);
}
