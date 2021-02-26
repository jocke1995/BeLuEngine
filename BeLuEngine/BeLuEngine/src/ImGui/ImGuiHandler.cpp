#include "stdafx.h"
#include "ImGuiHandler.h"

#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

#include "../Renderer/Statistics/EngineStatistics.h"
#include <psapi.h>

#include "../Renderer/Renderer.h"
#include "../Misc/Window.h"
#include "../Misc/Log.h"

#include "../Renderer/Camera/BaseCamera.h"

#include "../Events/EventBus.h"
#include "../Events/Events.h"

#include "../ECS/Entity.h"
#define DIV 1024

ImGuiHandler& ImGuiHandler::GetInstance()
{
	// Init
	EngineStatistics::GetInstance();
	static ImGuiHandler instance;

	return instance;
}

void ImGuiHandler::NewFrame()
{
	static bool a = true;

	if(a)
		EventBus::GetInstance().Subscribe(this, &ImGuiHandler::onEntityClicked);

	a = false;
	this->resetThreadInfos();

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiHandler::EndFrame()
{
	
}

void ImGuiHandler::UpdateFrame()
{
	// Update statistics
	this->updateMemoryInfo();

	Renderer& r = Renderer::GetInstance();
	const Window* window = r.GetWindow();

	IM_CommonStats& cStats = EngineStatistics::GetIM_CommonStats();
	IM_MemoryStats& mStats = EngineStatistics::GetIM_MemoryStats();
	std::vector<IM_ThreadStats*>& tStats = EngineStatistics::GetIM_ThreadStats();

	cStats.m_TotalFPS = ImGui::GetIO().Framerate;
	cStats.m_TotalMS = 1000.0f / ImGui::GetIO().Framerate;
	

	if (ImGui::Begin("Statistics"))
	{
		if (ImGui::CollapsingHeader("Common"))
		{
			ImGui::Text("Build: %s", cStats.m_Build.c_str());
			ImGui::Text("DebugLayer Active: %s", cStats.m_DebugLayerActive ? "yes" : "no");
			ImGui::Text("API: %s", cStats.m_API.c_str());
			ImGui::Text("Multithreaded Rendering: %s", cStats.m_STRenderer ? "no" : "yes");
			ImGui::Text("Adapter: %s", cStats.m_Adapter.c_str());
			ImGui::Text("CPU ID: %s", cStats.m_CPU.c_str());
			ImGui::Text("Total Cpu Cores: %d", cStats.m_NumCpuCores);
			ImGui::Text("Resolution: %d x %d", cStats.m_ResX, cStats.m_ResY);
			ImGui::Text("FPS: %d", cStats.m_TotalFPS);
			ImGui::Text("MS: %f", cStats.m_TotalMS);
		}
		if (ImGui::CollapsingHeader("Memory"))
		{
			ImGui::Text("RAM (MiB)");
 			ImGui::Text("Process usage: %d", mStats.m_ProcessRamUsage);
			ImGui::Text("Total usage: %d", mStats.m_TotalRamUsage);
			ImGui::Text("Installed: %d", mStats.m_TotalRam);
			ImGui::Text("----------------------");
			ImGui::Text("VRAM (MiB)");
			ImGui::Text("Process usage: %d", mStats.m_ProcessVramUsage);
			ImGui::Text("Installed: %d", mStats.m_TotalVram);
		}
		if (ImGui::CollapsingHeader("Threads"))
		{
			unsigned int threadsUsedThisFrame = 0;
			for (IM_ThreadStats* threadStat : tStats)
			{
				// Only show the threads who completed a task this frame
				if (threadStat->m_TasksCompleted != 0)
				{
					threadsUsedThisFrame++;
				}
			}

			ImGui::Text("CPU threads: %d", threadsUsedThisFrame);
			for (IM_ThreadStats* threadStat : tStats)
			{
				// Only show the threads who completed a task this frame
				if (threadStat->m_TasksCompleted != 0)
				{
					ImGui::Text("ID: %d\tCompleted tasks: %d", threadStat->m_Id, threadStat->m_TasksCompleted);
				}
			}
		}
	}
	ImGui::End();

	drawSelectedEntityInfo();
}

ImGuiHandler::ImGuiHandler()
{
#ifdef _DEBUG
	EngineStatistics::GetIM_CommonStats().m_Build = "Debug";
#elif DIST
	EngineStatistics::GetIM_CommonStats().m_Build = "Dist";
#else
	EngineStatistics::GetIM_CommonStats().m_Build = "Debug_Release";
#endif
}

ImGuiHandler::~ImGuiHandler()
{
	EventBus::GetInstance().Unsubscribe(this, &ImGuiHandler::onEntityClicked);
}

void ImGuiHandler::onEntityClicked(MouseClick* event)
{
	if (event->button == MOUSE_BUTTON::LEFT_DOWN && event->pressed == true)
	{
		Renderer& r = Renderer::GetInstance();

		Entity* e = r.GetPickedEntity();
		if (e != nullptr)
		{
			m_pSelectedEntity = e;
			// Do something..?
			Log::Print("Picked and pressed at: %s!\n", m_pSelectedEntity->GetName().c_str());
		}
	}
}

void ImGuiHandler::drawSelectedEntityInfo()
{
	Renderer& r = Renderer::GetInstance();
	const Window* window = r.GetWindow();

	if (m_pSelectedEntity == nullptr)
		return;
	std::string imGuiTitle = "Entity: " + m_pSelectedEntity->GetName();
	if (ImGui::Begin(imGuiTitle.c_str()))
	{
		for (Component* c : *m_pSelectedEntity->GetAllComponents())
		{
			if (dynamic_cast<component::ModelComponent*>(c))
			{
				if (ImGui::CollapsingHeader("Model"))
				{
					//ImGui::Text("Build: %s", cStats.m_Build.c_str());
				}
			}
			else if (dynamic_cast<component::TransformComponent*>(c))
			{
				if (ImGui::CollapsingHeader("Transform"))
				{
					//ImGui::Text("Build: %s", cStats.m_Build.c_str());
				}
			}
		}
	}
	ImGui::End();
}

void ImGuiHandler::updateMemoryInfo()
{
	Renderer& r = Renderer::GetInstance();
	IM_MemoryStats& mStats = EngineStatistics::GetIM_MemoryStats();

	// Ram usage
	PROCESS_MEMORY_COUNTERS pmc{};
	if (GetProcessMemoryInfo(r.m_ProcessHandle, &pmc, sizeof(pmc)))
	{
		MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memInfo);

		mStats.m_TotalRam = memInfo.ullTotalPhys / DIV / DIV;
		mStats.m_TotalRamUsage = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / DIV / DIV;
		mStats.m_ProcessRamUsage = (pmc.WorkingSetSize / DIV / DIV);
	}

	// Vram usage
	DXGI_QUERY_VIDEO_MEMORY_INFO info;
	if (SUCCEEDED(r.m_pAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
	{
		DXGI_ADAPTER_DESC3 aDesc = {};
		r.m_pAdapter4->GetDesc3(&aDesc);

		mStats.m_ProcessVramUsage = info.CurrentUsage / DIV / DIV;
		mStats.m_TotalVram = aDesc.DedicatedVideoMemory / DIV / DIV;
	};
}

void ImGuiHandler::resetThreadInfos()
{
	std::vector<IM_ThreadStats*>& tStats = EngineStatistics::GetIM_ThreadStats();

	for (IM_ThreadStats* threadStat : tStats)
	{
		// Only show the threads who completed a task this frame
		threadStat->m_TasksCompleted = 0;
	}
}
