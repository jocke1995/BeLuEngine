#include "stdafx.h"
#include "ImGUIRenderTask.h"

// ImGui stuff
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

#include "../ImGui/ImGuiHandler.h"

#include "../Misc/Window.h"

// API specific.. ImGui Requires stuff from manager
#include "../Renderer/API/D3D12/D3D12GraphicsManager.h"
#include "../Renderer/API/D3D12/D3D12DescriptorHeap.h"

// Generic API
#include "../Renderer/API/Interface/IGraphicsTexture.h"
#include "../Renderer/API/Interface/IGraphicsContext.h"

ImGuiRenderTask::ImGuiRenderTask(unsigned int screenWidth, unsigned int screenHeight)
	:GraphicsPass(L"ImGuiPass")
{
	E_GRAPHICS_API api = IGraphicsManager::GetGraphicsApiType();
	if (api == E_GRAPHICS_API::D3D12)
	{
		D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();
		D3D12DescriptorHeap* mainHeap = d3d12Manager->GetMainDescriptorHeap();

		// Setup ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		// Setup ImGui style
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouse;

		io.DisplaySize.x = screenWidth;
		io.DisplaySize.y = screenHeight;

		unsigned int imGuiTextureIndex = mainHeap->GetNextDescriptorHeapIndex(1);

		// Setup Platform/Renderer bindings
		ImGui_ImplWin32_Init(Window::GetInstance()->GetHwnd());
		ImGui_ImplDX12_Init(d3d12Manager->GetDevice(), NUM_SWAP_BUFFERS,
			DXGI_FORMAT_R16G16B16A16_FLOAT, mainHeap->GetID3D12DescriptorHeap(),
			mainHeap->GetCPUHeapAt(imGuiTextureIndex),
			mainHeap->GetGPUHeapAt(imGuiTextureIndex));
	}
	else if (api == E_GRAPHICS_API::VULKAN)
	{
		BL_ASSERT(false);
	}
}

ImGuiRenderTask::~ImGuiRenderTask()
{
	// Cleanup ImGui
	E_GRAPHICS_API api = IGraphicsManager::GetGraphicsApiType();
	if (api == E_GRAPHICS_API::D3D12)
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	else if (api == E_GRAPHICS_API::VULKAN)
	{
		BL_ASSERT(false);
	}
}

void ImGuiRenderTask::Execute()
{
	IGraphicsTexture* finalColorTexture = m_CommonGraphicsResources->finalColorBuffer;
	BL_ASSERT(finalColorTexture);

	ImGuiHandler& imGuiHandler = ImGuiHandler::GetInstance();

	// Start the new frame
	imGuiHandler.NewFrame();

	// Update ImGui here to get all information that happens inside rendering
	imGuiHandler.UpdateFrame();

	// Context-recording
	{
		m_pGraphicsContext->Begin();
		{
			ScopedPixEvent(ImGuiPass, m_pGraphicsContext);

			m_pGraphicsContext->SetupBindings(false);

			m_pGraphicsContext->ResourceBarrier(finalColorTexture, BL_RESOURCE_STATE_RENDER_TARGET);
			m_pGraphicsContext->SetRenderTargets(1, &finalColorTexture, nullptr);

			m_pGraphicsContext->DrawImGui();
		}
		m_pGraphicsContext->End();
	}

	// End the frame
	imGuiHandler.EndFrame();
}
