#include "stdafx.h"
#include "ImGUIRenderTask.h"

// Generic API
#include "../Renderer/API/IGraphicsTexture.h"
#include "../Renderer/API/IGraphicsContext.h"

ImGuiRenderTask::ImGuiRenderTask()
	:GraphicsPass(L"ImGuiPass")
{
}

ImGuiRenderTask::~ImGuiRenderTask()
{
}

void ImGuiRenderTask::Execute()
{
	IGraphicsTexture* finalColorTexture = m_GraphicTextures["finalColorBuffer"];
	BL_ASSERT(finalColorTexture);

	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(ImGuiPass, m_pGraphicsContext);

		m_pGraphicsContext->SetupBindings(false);

		m_pGraphicsContext->SetRenderTargets(1, &finalColorTexture, nullptr);

		m_pGraphicsContext->DrawImGui();
	}
	m_pGraphicsContext->End();
}
