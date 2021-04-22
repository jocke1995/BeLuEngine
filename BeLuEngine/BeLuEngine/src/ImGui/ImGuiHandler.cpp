#include "stdafx.h"
#include "ImGuiHandler.h"

#include "../ImGUI/imgui.h"
#include "../ImGui/imgui_internal.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

#include "../Renderer/Statistics/EngineStatistics.h"
#include <psapi.h>

#include "../Renderer/Renderer.h"
#include "../Misc/Window.h"
#include "../Misc/Log.h"
#include "../Misc/AssetLoader.h"

#include "../Renderer/Camera/BaseCamera.h"

#include "../Events/EventBus.h"
#include "../Events/Events.h"

// ECS
#include "../ECS/Entity.h"
#include "../ECS/Scene.h"

// Geometry
#include "../Renderer/Model/Transform.h"
#include "../Renderer/Model/Material.h"

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

	drawSceneHierarchy();
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
		}
	}
}

void ImGuiHandler::drawSceneHierarchy()
{
	Renderer& r = Renderer::GetInstance();
	const Window* window = r.GetWindow();

	if (ImGui::Begin("Scene"))
	{
		if (ImGui::BeginTable("", 1))
		{
			for (auto& pair : *r.GetActiveScene()->GetEntities())
			{
				std::string imGuiTitle = "Entity: " + pair.second->GetName();

				int headerFlags = 0;
				if (pair.second == m_pSelectedEntity)
				{
					headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
				}

				if (ImGui::TableNextColumn() && ImGui::CollapsingHeader(imGuiTitle.c_str(), headerFlags))
				{
					if (ImGui::BeginTable("Components", 1))
					{
						ImGui::Indent();
						for (Component* c : *pair.second->GetAllComponents())
						{
							if (component::ModelComponent* mc = dynamic_cast<component::ModelComponent*>(c))
							{
								if (ImGui::TableNextColumn() && ImGui::CollapsingHeader("Model"))
								{
									static int matIndex = 0;
									ImGui::InputInt("Mesh Index", &matIndex);

									// Sanity check
									if (matIndex > (mc->GetNrOfMeshes() - 1) || matIndex < 0)
									{
										matIndex = mc->GetNrOfMeshes() - 1;
									}

									Material* mat = mc->GetMaterialAt(matIndex);
									MaterialData* matData = const_cast<MaterialData*>(&mat->GetMaterialData()->second);

									bool useRoughnessTexture = matData->hasRoughnessTexture;
									bool useMetallicTexture = matData->hasMetallicTexture;
									bool useOpacityTexture = matData->hasOpacityTexture;
									bool useEmissiveTexture = matData->hasEmissiveTexture;
									bool useNormalTexture = matData->hasNormalTexture;
									bool glow = matData->glow;

									float roughnessValue = matData->roughnessValue;
									float metallicValue = matData->metallicValue;
									float opacityValue = matData->opacityValue;
									float4 emissiveValue = matData->emissiveValue;

									bool isPressed = ImGui::Button("Create Unique Material", { 200.0f, 100.0f });
									ImGui::Checkbox("Use Roughness Texture", &useRoughnessTexture);
									ImGui::Checkbox("Use Metallic Texture", &useMetallicTexture);
									ImGui::Checkbox("Use Opacity Texture", &useOpacityTexture);
									ImGui::Checkbox("Use Emissive Texture", &useEmissiveTexture);
									ImGui::Checkbox("Use Normal Texture", &useNormalTexture);
									ImGui::Checkbox("Glow", &glow);

									ImGui::DragFloat("Roughness", &roughnessValue, 0.005f, 0.01f, 1.0f);
									ImGui::DragFloat("Metallic", &metallicValue, 0.005f, 0.01f, 1.0f);
									ImGui::DragFloat("Opacity", &opacityValue, 0.005f, 0.01f, 1.0f);

									if (useEmissiveTexture == false)
									{
										ImGui::ColorEdit3("Emissive Color", &emissiveValue.r);
										ImGui::DragFloat("Emissive Intensity", &emissiveValue.a, 0.1f, 0.0f, 10.0f);
										matData->emissiveValue = emissiveValue;
									}

									if (isPressed)
									{
										static int index = 0;
										AssetLoader* al = AssetLoader::Get();
										std::wstring matName = L"dynamicMaterial" + std::to_wstring(index);
										Material* newMat = al->CreateMaterial(matName);
										index++;

										mc->SetMaterialAt(matIndex, newMat);
									}
									matData->hasRoughnessTexture = useRoughnessTexture;
									matData->hasMetallicTexture = useMetallicTexture;
									matData->hasOpacityTexture = useOpacityTexture;
									matData->hasEmissiveTexture = useEmissiveTexture;
									matData->hasNormalTexture = useNormalTexture;
									matData->glow = glow;

									matData->roughnessValue = roughnessValue;
									matData->metallicValue = metallicValue;
									matData->opacityValue = opacityValue;
									// Update data on VRAM
									r.submitMaterialDataToGPU(mat);
								}
							}
							else if (component::TransformComponent* tc = dynamic_cast<component::TransformComponent*>(c))
							{
								if (ImGui::TableNextColumn() && ImGui::CollapsingHeader("Transform"))
								{
									DirectX::XMFLOAT3 pos = tc->GetTransform()->GetPositionXMFLOAT3();
									DirectX::XMFLOAT3 scale = tc->GetTransform()->GetScale();
									DirectX::XMFLOAT3 rot = tc->GetTransform()->GetRot();

									ImGui::Text("Translate");
									ImGui::DragFloat("##X1", &pos.x, 0.1f);
									ImGui::DragFloat("##Y1", &pos.y, 0.1f);
									ImGui::DragFloat("##Z1", &pos.z, 0.1f);
									ImGui::Text("Scale");
									ImGui::DragFloat("##X2", &scale.x, 0.1f);
									ImGui::DragFloat("##Y2", &scale.y, 0.1f);
									ImGui::DragFloat("##Z2", &scale.z, 0.1f);
									ImGui::Text("Rot");
									ImGui::DragFloat("##X3", &rot.x, 0.1f);
									ImGui::DragFloat("##Y3", &rot.y, 0.1f);
									ImGui::DragFloat("##Z3", &rot.z, 0.1f);

									// Nice consistensy :)
									tc->GetTransform()->SetPosition(pos);
									tc->GetTransform()->SetScale(scale.x, scale.y, scale.z);
									tc->GetTransform()->SetRotationX(rot.x);
									tc->GetTransform()->SetRotationY(rot.y);
									tc->GetTransform()->SetRotationZ(rot.z);
								}
							}
							else if (component::PointLightComponent* plc = dynamic_cast<component::PointLightComponent*>(c))
							{
								if (ImGui::TableNextColumn() && ImGui::CollapsingHeader("Point Light"))
								{
									PointLight* pl = static_cast<PointLight*>(plc->GetLightData());

									ImGui::DragFloat("Intensity", &pl->baseLight.intensity, 0.1f, 0.0f, 10.0f);
									ImGui::ColorEdit3("", &pl->baseLight.color.r);
								}
							}
							else if (component::SpotLightComponent* slc = dynamic_cast<component::SpotLightComponent*>(c))
							{
								if (ImGui::TableNextColumn() && ImGui::CollapsingHeader("Spot Light"))
								{
									SpotLight* sl = static_cast<SpotLight*>(slc->GetLightData());

									ImGui::DragFloat("Intensity", &sl->baseLight.intensity, 0.1f, 0.0f, 10.0f);
									ImGui::DragFloat3("Direction", &sl->direction_outerCutoff.r, 0.05f, -1.0f, 1.0f);
									ImGui::ColorEdit3("", &sl->baseLight.color.r);
								}
							}
							else if (component::DirectionalLightComponent* dlc = dynamic_cast<component::DirectionalLightComponent*>(c))
							{
								if (ImGui::TableNextColumn() && ImGui::CollapsingHeader("Directional Light"))
								{
									DirectionalLight* dl = static_cast<DirectionalLight*>(dlc->GetLightData());

									ImGui::DragFloat("Intensity", &dl->baseLight.intensity, 0.1f, 0.0f, 10.0f);
									ImGui::DragFloat3("Direction", &dl->direction.r, 0.05f, -1.0f, 1.0f);
									ImGui::ColorEdit3("", &dl->baseLight.color.r);
								}
							}
						}
						ImGui::Unindent();
					}
					ImGui::EndTable();
				}
			}
		}
		ImGui::EndTable();
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
