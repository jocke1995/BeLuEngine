#include "stdafx.h"
#include "TopLevelRenderTask.h"

#include "../DXR/TopLevelAccelerationStructure.h"

// Generic API
#include "../API/IGraphicsContext.h"

TopLevelRenderTask::TopLevelRenderTask()
	:GraphicsPass(L"DXR_TopLevelASPass")
{
	m_pTLAS = new TopLevelAccelerationStructure();
}

TopLevelRenderTask::~TopLevelRenderTask()
{
	BL_SAFE_DELETE(m_pTLAS);
}

void TopLevelRenderTask::Execute()
{
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DXR_TLAS, m_pGraphicsContext);

		m_pGraphicsContext->BuildAccelerationStructure(m_pTLAS->GetBuildDesc());
		m_pGraphicsContext->UAVBarrier(m_pTLAS->GetRayTracingResultBuffer());

		m_pTLAS->m_IsBuilt = true;
	}
	m_pGraphicsContext->End();
}

TopLevelAccelerationStructure* TopLevelRenderTask::GetTLAS() const
{
	return m_pTLAS;
}

