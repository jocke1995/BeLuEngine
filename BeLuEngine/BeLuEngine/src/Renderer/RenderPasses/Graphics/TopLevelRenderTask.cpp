#include "stdafx.h"
#include "TopLevelRenderTask.h"

// ECS
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/TransformComponent.h"
#include "../../Geometry/Model.h"
#include "../../Geometry/Transform.h"

// Renderer
#include "../Renderer/Renderer.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsContext.h"
#include "../Renderer/API/Interface/RayTracing/ITopLevelAS.h"

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
	// Add topLevel instances to the resultBuffer
	m_pTLAS->MapResultBuffer();

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(DXR_TLAS, m_pGraphicsContext);

		m_pGraphicsContext->BuildTLAS(m_pTLAS);
	}
	m_pGraphicsContext->End();
}

void TopLevelRenderTask::SetRenderComponents(const std::vector<RenderComponent>& renderComponents)
{
	m_RenderComponents = renderComponents;
}

ITopLevelAS* TopLevelRenderTask::GetTopLevelAS() const
{
	return m_pTLAS;;
}
