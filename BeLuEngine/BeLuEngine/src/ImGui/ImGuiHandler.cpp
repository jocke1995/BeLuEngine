#include "stdafx.h"
#include "ImGuiHandler.h"

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

#include "../Renderer/Statistics/RenderData.h"
#include <psapi.h>

#include "../Renderer/Renderer.h"
#include "../Misc/Window.h"

ImGuiHandler& ImGuiHandler::GetInstance()
{
	static ImGuiHandler instance;

	return instance;
}

void ImGuiHandler::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiHandler::UpdateFrame()
{
	Renderer& r = Renderer::GetInstance();
	const Window* window = r.GetWindow();

	EngineStatistics::GetIM_RenderStats().m_TotalFPS = ImGui::GetIO().Framerate;
	EngineStatistics::GetIM_RenderStats().m_TotalMS = 1000.0f / ImGui::GetIO().Framerate;
	
	IM_CommonStats& cstats = EngineStatistics::GetIM_RenderStats();

	if (ImGui::Begin("Statistics"))
	{
		if (ImGui::CollapsingHeader("Common"))
		{
			ImGui::Text("Build: %s", cstats.m_Build.c_str());
			ImGui::Text("DebugLayer Active: %s", cstats.m_DebugLayerActive ? "yes" : "no");
			ImGui::Text("API: %s", cstats.m_API.c_str());
			ImGui::Text("Multithreading commandlists: %s", cstats.m_STRenderer ? "no" : "yes");
			ImGui::Text("Adapter: %s", cstats.m_Adapter.c_str());
			ImGui::Text("Resolution: %d x %d", cstats.m_ResX, cstats.m_ResY);
			ImGui::Text("FPS: %d", cstats.m_TotalFPS);
			ImGui::Text("MS: %f", cstats.m_TotalMS);
		}
		if (ImGui::CollapsingHeader("Memory"))
		{
			// Vram usage
			DXGI_QUERY_VIDEO_MEMORY_INFO info;
			if (SUCCEEDED(r.m_pAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
			{
				float memoryUsage = float(info.CurrentUsage / 1024.0 / 1024.0); //MiB
				ImGui::Text("VRAM usage: %.02f MiB", memoryUsage);
			};

			// Ram usage
			PROCESS_MEMORY_COUNTERS pmc{};
			if (GetProcessMemoryInfo(r.m_ProcessHandle, &pmc, sizeof(pmc)))
			{
				//PagefileUsage is the:
					//The Commit Charge value in bytes for this process.
					//Commit Charge is the total amount of memory that the memory manager has committed for a running process.
				float memoryUsage = float(pmc.WorkingSetSize / 1024.0 / 1024.0); //MiB
				ImGui::Text("RAM usage: %.02f MiB", memoryUsage);
			}
		}
		
	}
	ImGui::End();
		
}

ImGuiHandler::ImGuiHandler()
{
#ifdef _DEBUG
	EngineStatistics::GetIM_RenderStats().m_Build = "Debug";
#elif DIST
	EngineStatistics::GetIM_RenderStats().m_Build = "Dist";
#else
	EngineStatistics::GetIM_RenderStats().m_Build = "Debug_Release";
#endif
}

ImGuiHandler::~ImGuiHandler()
{
}
