#include "stdafx.h"
#include "ImGuiHandler.h"

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

#include "../Renderer/Statistics/RenderData.h"
#include <psapi.h>

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
	EngineStatistics::GetIM_RenderStats().m_TotalFPS = ImGui::GetIO().Framerate;
	EngineStatistics::GetIM_RenderStats().m_TotalMS = 1000.0f / ImGui::GetIO().Framerate;

#pragma region RenderStats
	ImGui::Begin("RenderStats");
	IM_RenderStats& stats = EngineStatistics::GetIM_RenderStats();

		ImGui::Text("Adapter: %s"		 , stats.m_Adapter.c_str());
		ImGui::Text("API: %s"			 , stats.m_API.c_str());
		ImGui::Text("Resolution: %d x %d", stats.m_ResX, stats.m_ResY);
		ImGui::Text("FPS: %d"			 , stats.m_TotalFPS);
		ImGui::Text("MS: %f"			 , stats.m_TotalMS);
	ImGui::End();
#pragma endregion RenderStats
}

ImGuiHandler::ImGuiHandler()
{
}

ImGuiHandler::~ImGuiHandler()
{
}
