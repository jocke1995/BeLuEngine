#include "stdafx.h"
#include "TopLevelRenderTask.h"

// Generic API
#include "../Renderer/API/IGraphicsContext.h"
#include "../Renderer/API/ITopLevelAS.h"

TopLevelRenderTask::TopLevelRenderTask()
	:GraphicsPass(L"DXR_TopLevelASPass")
{
	m_pTLAS = ITopLevelAS::Create();
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

		m_pGraphicsContext->BuildTLAS(m_pTLAS);
	}
	m_pGraphicsContext->End();
}

ITopLevelAS* TopLevelRenderTask::GetTLAS() const
{
	return m_pTLAS;
}
