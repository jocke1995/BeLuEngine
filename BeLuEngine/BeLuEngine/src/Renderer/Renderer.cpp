#include "stdafx.h"
#include "Renderer.h"

// Misc
#include "../Misc/MultiThreading/ThreadPool.h"
#include "../Misc/AssetLoader.h"
#include "../Misc/MultiThreading/Thread.h"
#include "../Misc/Window.h"
// ECS
#include "../ECS/Scene.h"
#include "../ECS/Entity.h"
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/BoundingBoxComponent.h"
#include "../ECS/Components/CameraComponent.h"
#include "../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../ECS/Components/Lights/PointLightComponent.h"
#include "../ECS/Components/Lights/SpotLightComponent.h"


// Renderer-Engine 
#include "RootSignature.h"
#include "SwapChain.h"
#include "GPUMemory/DepthStencilView.h"
#include "ViewPool.h"
#include "BoundingBoxPool.h"
#include "CommandInterface.h"
#include "DescriptorHeap.h"
#include "Transform.h"
#include "Camera/BaseCamera.h"
#include "Model.h"
#include "Mesh.h"
#include "Texture/Texture.h"
#include "Texture/Texture2D.h"
#include "Texture/Texture2DGUI.h"
#include "Texture/TextureCubeMap.h"
#include "Material.h"

// GPUMemory
#include "GPUMemory/Resource.h"
#include "GPUMemory/ConstantBuffer.h"
#include "GPUMemory/UnorderedAccess.h"
// Views
#include "GPUMemory/ShaderResourceView.h"
#include "GPUMemory/ConstantBufferView.h"
#include "GPUMemory/DepthStencil.h"

// Techniques
#include "Bloom.h"
#include "PingPongResource.h"
#include "ShadowInfo.h"
#include "MousePicker.h"

// Graphics
#include "DX12Tasks/DepthRenderTask.h"
#include "DX12Tasks/WireframeRenderTask.h"
#include "DX12Tasks/OutliningRenderTask.h"
#include "DX12Tasks/ForwardRenderTask.h"
#include "DX12Tasks/TransparentRenderTask.h"
#include "DX12Tasks/ShadowRenderTask.h"
#include "DX12Tasks/DownSampleRenderTask.h"
#include "DX12Tasks/MergeRenderTask.h"

// Copy 
#include "DX12Tasks/CopyPerFrameTask.h"
#include "DX12Tasks/CopyOnDemandTask.h"

// Compute
#include "DX12Tasks/BlurComputeTask.h"

// Event
#include "../Events/EventBus.h"

Renderer::Renderer()
{
	EventBus::GetInstance().Subscribe(this, &Renderer::toggleFullscreen);
	m_RenderTasks.resize(RENDER_TASK_TYPE::NR_OF_RENDERTASKS);
	m_CopyTasks.resize(COPY_TASK_TYPE::NR_OF_COPYTASKS);
	m_ComputeTasks.resize(COMPUTE_TASK_TYPE::NR_OF_COMPUTETASKS);
}

Renderer& Renderer::GetInstance()
{
	static Renderer instance;
	return instance;
}

Renderer::~Renderer()
{
	
}

void Renderer::deleteRenderer()
{
	Log::Print("----------------------------  Deleting Renderer  ----------------------------------\n");
	waitForGPU();

	SAFE_RELEASE(&m_pFenceFrame);
	if (!CloseHandle(m_EventHandle))
	{
		Log::Print("Failed To Close Handle... ErrorCode: %d\n", GetLastError());
	}

	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]);
	SAFE_RELEASE(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]);

	delete m_pRootSignature;
	delete m_pFullScreenQuad;
	delete m_pSwapChain;
	delete m_pBloomResources;
	delete m_pMainDepthStencil;

	for (auto& pair : m_DescriptorHeaps)
	{
		delete pair.second;
	}

	for (ComputeTask* computeTask : m_ComputeTasks)
		delete computeTask;

	for (CopyTask* copyTask : m_CopyTasks)
		delete copyTask;

	for (RenderTask* renderTask : m_RenderTasks)
		delete renderTask;

	SAFE_RELEASE(&m_pDevice5);

	delete m_pMousePicker;

	delete m_pViewPool;
	delete m_pCbPerScene;
	delete m_pCbPerSceneData;
	delete m_pCbPerFrame;
	delete m_pCbPerFrameData;
}

void Renderer::InitD3D12(Window *window, HINSTANCE hInstance, ThreadPool* threadPool)
{
	m_pThreadPool = threadPool;
	m_pWindow = window;

	// Create Device
	if (!createDevice())
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to Create Device\n");
	}

	// Create CommandQueues (copy, compute and direct)
	createCommandQueues();

	// Create DescriptorHeaps
	createDescriptorHeaps();

	// Fence for WaitForFrame();
	createFences();

	// Rendertargets
	createSwapChain();
	m_pBloomResources = new Bloom(m_pDevice5, 
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
		m_pSwapChain
		);

	// Create Main DepthBuffer
	createMainDSV();

	// Picking
	m_pMousePicker = new MousePicker();
	
	// Create Rootsignature
	createRootSignature();

	// FullScreenQuad
	createFullScreenQuad();
	// Init Assetloader
	AssetLoader* al = AssetLoader::Get(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV], m_pWindow);

	// Init BoundingBoxPool
	BoundingBoxPool::Get(m_pDevice5, m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
	
	// Pool to handle GPU memory for the lights
	m_pViewPool = new ViewPool(
		m_pDevice5,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV]);

	// Allocate memory for cbPerScene
	m_pCbPerScene = new ConstantBuffer(
		m_pDevice5, 
		sizeof(CB_PER_SCENE_STRUCT),
		L"CB_PER_SCENE",
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]
		);
	
	m_pCbPerSceneData = new CB_PER_SCENE_STRUCT();

	// Allocate memory for cbPerFrame
	m_pCbPerFrame = new ConstantBuffer(
		m_pDevice5,
		sizeof(CB_PER_FRAME_STRUCT),
		L"CB_PER_FRAME",
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]
	);

	m_pCbPerFrameData = new CB_PER_FRAME_STRUCT();

	initRenderTasks();
	
	submitMeshToCodt(m_pFullScreenQuad);
}

void Renderer::Update(double dt)
{
	float3 right = m_pScenePrimaryCamera->GetRightVectorFloat3();
	right.normalize();

	float3 forward = m_pScenePrimaryCamera->GetDirectionFloat3();
	forward.normalize();

	// TODO: fix camera up vector
	float3 up = forward.cross(right);
	up.normalize();

	// Update CB_PER_FRAME data
	m_pCbPerFrameData->camPos = m_pScenePrimaryCamera->GetPositionFloat3();
	m_pCbPerFrameData->camRight = right;
	m_pCbPerFrameData->camUp = up;
	m_pCbPerFrameData->camForward = forward;

	// Picking
	updateMousePicker();
}

void Renderer::SortObjects()
{
	struct DistFromCamera
	{
		double distance;
		component::ModelComponent* mc;
		component::TransformComponent* tc;
	};

	for (auto& renderComponents : m_RenderComponents)
	{
		int numRenderComponents = renderComponents.second.size();

		DistFromCamera* distFromCamArr = new DistFromCamera[numRenderComponents];

		// Get all the distances of each objects and store them by ID and distance
		DirectX::XMFLOAT3 camPos = m_pScenePrimaryCamera->GetPosition();
		for (int i = 0; i < numRenderComponents; i++)
		{
			DirectX::XMFLOAT3 objectPos = renderComponents.second.at(i).second->GetTransform()->GetPositionXMFLOAT3();

			double distance = sqrt(pow(camPos.x - objectPos.x, 2) +
				pow(camPos.y - objectPos.y, 2) +
				pow(camPos.z - objectPos.z, 2));

			// Save the object alongside its distance to the m_pCamera
			distFromCamArr[i].distance = distance;
			distFromCamArr[i].mc = renderComponents.second.at(i).first;
			distFromCamArr[i].tc = renderComponents.second.at(i).second;
		}

		// InsertionSort (because its best case is O(N)), 
		// and since this is sorted ((((((EVERY FRAME)))))) this is a good choice of sorting algorithm
		int j = 0;
		DistFromCamera distFromCamArrTemp = {};
		for (int i = 1; i < numRenderComponents; i++)
		{
			j = i;
			while (j > 0 && (distFromCamArr[j - 1].distance > distFromCamArr[j].distance))
			{
				// Swap
				distFromCamArrTemp = distFromCamArr[j - 1];
				distFromCamArr[j - 1] = distFromCamArr[j];
				distFromCamArr[j] = distFromCamArrTemp;
				j--;
			}
		}

		// Fill the vector with sorted array
		renderComponents.second.clear();
		for (int i = 0; i < numRenderComponents; i++)
		{
			renderComponents.second.push_back(std::make_pair(distFromCamArr[i].mc, distFromCamArr[i].tc));
		}

		// Free memory
		delete distFromCamArr;
	}

	// Update the entity-arrays inside the rendertasks
	setRenderTasksRenderComponents();
}

void Renderer::Execute()
{
	IDXGISwapChain4* dx12SwapChain = m_pSwapChain->GetDX12SwapChain();
	int backBufferIndex = dx12SwapChain->GetCurrentBackBufferIndex();
	int commandInterfaceIndex = m_FrameCounter++ % 2;

	CopyTask* copyTask = nullptr;
	ComputeTask* computeTask = nullptr;
	RenderTask* renderTask = nullptr;
	/* --------------------- Record command lists --------------------- */

	// Copy on demand
	copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	copyTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(copyTask);

	// Copy per frame
	copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	copyTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(copyTask);

	// Depth pre-pass
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS];
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// Recording shadowmaps
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::SHADOW];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// Opaque draw
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// DownSample the texture used for bloom
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::DOWNSAMPLE];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// Blending with constant value
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_CONSTANT];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// Blending with opacity texture
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_TEXTURE];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	// Blurring for bloom
	computeTask = m_ComputeTasks[COMPUTE_TASK_TYPE::BLUR];
	computeTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(computeTask);

	// Outlining, if an object is picked
	renderTask = m_RenderTasks[RENDER_TASK_TYPE::OUTLINE];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);

	renderTask = m_RenderTasks[RENDER_TASK_TYPE::MERGE];
	renderTask->SetBackBufferIndex(backBufferIndex);
	renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
	m_pThreadPool->AddTask(renderTask);
	
	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		renderTask = m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME];
		renderTask->SetBackBufferIndex(backBufferIndex);
		renderTask->SetCommandInterfaceIndex(commandInterfaceIndex);
		m_pThreadPool->AddTask(renderTask);
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */

	// Wait for the threads which records the commandlists to complete
	m_pThreadPool->WaitForThreads(FLAG_THREAD::RENDER);

	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->ExecuteCommandLists(
		m_DirectCommandLists[commandInterfaceIndex].size(), 
		m_DirectCommandLists[commandInterfaceIndex].data());

	/* --------------------------------------------------------------- */

	// Wait if the CPU is to far ahead of the gpu
	m_FenceFrameValue++;

	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);
	waitForFrame();

	/*------------------- Post draw stuff -------------------*/
	// Clear copy on demand
	m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]->Clear();

	/*------------------- Present -------------------*/
	HRESULT hr = dx12SwapChain->Present(0, 0);
	
#ifdef _DEBUG
	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Swapchain Failed to present\n");
	}
#endif
}

void Renderer::InitModelComponent(component::ModelComponent* mc)
{
	component::TransformComponent* tc = mc->GetParent()->GetComponent<component::TransformComponent>();

	// Submit to codt
	submitModelToGPU(mc->m_pModel);
	
	// Only add the m_Entities that actually should be drawn
	if (tc != nullptr)
	{
		// Finally store the object in the corresponding renderComponent vectors so it will be drawn
		if (FLAG_DRAW::DRAW_TRANSPARENT_CONSTANT & mc->GetDrawFlag())
		{
			m_RenderComponents[FLAG_DRAW::DRAW_TRANSPARENT_CONSTANT].push_back(std::make_pair(mc, tc));
		}

		if (FLAG_DRAW::DRAW_TRANSPARENT_TEXTURE & mc->GetDrawFlag())
		{
			m_RenderComponents[FLAG_DRAW::DRAW_TRANSPARENT_TEXTURE].push_back(std::make_pair(mc, tc));
		}

		if (FLAG_DRAW::DRAW_OPAQUE & mc->GetDrawFlag())
		{
			m_RenderComponents[FLAG_DRAW::DRAW_OPAQUE].push_back(std::make_pair(mc, tc));
		}

		if (FLAG_DRAW::NO_DEPTH & ~mc->GetDrawFlag())
		{
			m_RenderComponents[FLAG_DRAW::NO_DEPTH].push_back(std::make_pair(mc, tc));
		}

		if (FLAG_DRAW::GIVE_SHADOW & mc->GetDrawFlag())
		{
			m_RenderComponents[FLAG_DRAW::GIVE_SHADOW].push_back(std::make_pair(mc, tc));
		}
	}
}

void Renderer::InitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"DirectionalLight";
	ConstantBuffer* cb = m_pViewPool->GetFreeCB(sizeof(DirectionalLight), resourceName);

	// Check if the light is to cast shadows
	SHADOW_RESOLUTION resolution = SHADOW_RESOLUTION::UNDEFINED;

	int shadowRes = -1;
	if (component->GetLightFlags() & FLAG_LIGHT::CAST_SHADOW)
	{
		// Max resolution
		shadowRes = 2;
	}

	if (shadowRes == 0)
	{
		resolution = SHADOW_RESOLUTION::LOW;
	}
	else if (shadowRes == 1)
	{
		resolution = SHADOW_RESOLUTION::MEDIUM;
	}
	else if (shadowRes >= 2)
	{
		resolution = SHADOW_RESOLUTION::HIGH;
	}

	// Assign views required for shadows from the lightPool
	ShadowInfo* si = nullptr;
	if (resolution != SHADOW_RESOLUTION::UNDEFINED)
	{
		si = m_pViewPool->GetFreeShadowInfo(LIGHT_TYPE::DIRECTIONAL_LIGHT, resolution);
		static_cast<DirectionalLight*>(component->GetLightData())->textureShadowMap = si->GetSRV()->GetDescriptorHeapIndex();

		ShadowRenderTask* srt = static_cast<ShadowRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::SHADOW]);
		srt->AddShadowCastingLight(std::make_pair(component, si));
	}

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].push_back(std::make_tuple(component, cb, si));

	
	// Submit to gpu
	CopyTask* copyTask = nullptr;

	if (component->GetLightFlags() & FLAG_LIGHT::STATIC)
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));
	
	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitPointLightComponent(component::PointLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"PointLight";
	ConstantBuffer* cb = m_pViewPool->GetFreeCB(sizeof(PointLight), resourceName);

	// Assign views required for shadows from the lightPool
	ShadowInfo* si = nullptr;

	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::POINT_LIGHT].push_back(std::make_tuple(component, cb, si));

	// Submit to gpu
	CopyTask* copyTask = nullptr;
	if (component->GetLightFlags() & FLAG_LIGHT::STATIC)
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));

	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitSpotLightComponent(component::SpotLightComponent* component)
{
	// Assign CBV from the lightPool
	std::wstring resourceName = L"SpotLight";
	ConstantBuffer* cb = m_pViewPool->GetFreeCB(sizeof(SpotLight), resourceName);

	// Check if the light is to cast shadows
	SHADOW_RESOLUTION resolution = SHADOW_RESOLUTION::UNDEFINED;

	int shadowRes = -1;
	if (component->GetLightFlags() & FLAG_LIGHT::CAST_SHADOW)
	{
		// Max resolution
		shadowRes = 2;
	}

	if (shadowRes == 0)
	{
		resolution = SHADOW_RESOLUTION::LOW;
	}
	else if (shadowRes == 1)
	{
		resolution = SHADOW_RESOLUTION::MEDIUM;
	}
	else if (shadowRes >= 2)
	{
		resolution = SHADOW_RESOLUTION::HIGH;
	}

	// Assign views required for shadows from the lightPool
	ShadowInfo* si = nullptr;
	if (resolution != SHADOW_RESOLUTION::UNDEFINED)
	{
		si = m_pViewPool->GetFreeShadowInfo(LIGHT_TYPE::SPOT_LIGHT, resolution);
		static_cast<SpotLight*>(component->GetLightData())->textureShadowMap = si->GetSRV()->GetDescriptorHeapIndex();

		ShadowRenderTask* srt = static_cast<ShadowRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::SHADOW]);
		srt->AddShadowCastingLight(std::make_pair(component, si));
	}
	// Save in m_pRenderer
	m_Lights[LIGHT_TYPE::SPOT_LIGHT].push_back(std::make_tuple(component, cb, si));

	// Submit to gpu
	CopyTask* copyTask = nullptr;
	if (component->GetLightFlags() & FLAG_LIGHT::STATIC)
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND];
	}
	else
	{
		copyTask = m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME];
	}

	const void* data = static_cast<const void*>(component->GetLightData());
	copyTask->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), data));

	// We also need to update the indexBuffer with lights if a light is added.
	// The buffer with indices is inside cbPerSceneData, which is updated in the following function:
	submitUploadPerSceneData();
}

void Renderer::InitCameraComponent(component::CameraComponent* component)
{
	if (component->IsPrimary() == true)
	{
		m_pScenePrimaryCamera = component->GetCamera();
	}
}

void Renderer::InitBoundingBoxComponent(component::BoundingBoxComponent* component)
{
	// Add it to m_pTask so it can be drawn
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		for (unsigned int i = 0; i < component->GetNumBoundingBoxes(); i++)
		{
			auto[mesh, toBeSubmitted] = BoundingBoxPool::Get()->CreateBoundingBoxMesh(component->GetPathOfModel(i));

			if (mesh == nullptr)
			{
				Log::PrintSeverity(Log::Severity::WARNING, "Forgot to initialize BoundingBoxComponent on Entity: %s\n", component->GetParent()->GetName().c_str());
				return;
			}

			if (toBeSubmitted == true)
			{
				submitMeshToCodt(mesh);
			}

			component->AddMesh(mesh);
		}
		static_cast<WireframeRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME])->AddObjectToDraw(component);
	}

	// Add to vector so the mouse picker can check for intersections
	if (component->GetFlagOBB() & F_OBBFlags::PICKING)
	{
		m_BoundingBoxesToBePicked.push_back(component);
	}
}

void Renderer::UnInitModelComponent(component::ModelComponent* component)
{
	// Remove component from renderComponents
	// TODO: change data structure to allow O(1) add and remove
	for (auto& renderComponent : m_RenderComponents)
	{
		for (int i = 0; i < renderComponent.second.size(); i++)
		{
			// Remove from all renderComponent-vectors if they are there
			component::ModelComponent* comp = nullptr;
			comp = renderComponent.second[i].first;
			if (comp == component)
			{
				renderComponent.second.erase(renderComponent.second.begin() + i);
			}
		}
	}

	// Update Render Tasks components (forward the change in renderComponents)
	setRenderTasksRenderComponents();
}

void Renderer::UnInitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::DIRECTIONAL_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::DirectionalLightComponent* dlc = static_cast<component::DirectionalLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == dlc)
		{
			// Free memory so other m_Entities can use it
			ConstantBuffer* cbv = std::get<1>(tuple);
			ShadowInfo* si = std::get<2>(tuple);
			m_pViewPool->ClearSpecificLight(type, cbv, si);

			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cbv->GetUploadResource());

			// Finally remove from m_pRenderer
			ShadowRenderTask* srt = static_cast<ShadowRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::SHADOW]);
			srt->ClearSpecificLight(std::get<0>(tuple));
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitPointLightComponent(component::PointLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::POINT_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::PointLightComponent* plc = static_cast<component::PointLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == plc)
		{
			// Free memory so other m_Entities can use it
			ConstantBuffer* cbv = std::get<1>(tuple);
			m_pViewPool->ClearSpecificLight(type, cbv, nullptr);

			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cbv->GetUploadResource());

			// Finally remove from m_pRenderer
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitSpotLightComponent(component::SpotLightComponent* component)
{
	LIGHT_TYPE type = LIGHT_TYPE::SPOT_LIGHT;
	unsigned int count = 0;
	for (auto& tuple : m_Lights[type])
	{
		Light* light = std::get<0>(tuple);

		component::SpotLightComponent* slc = static_cast<component::SpotLightComponent*>(light);

		// Remove light if it matches the entity
		if (component == slc)
		{
			// Free memory so other m_Entities can use it
			ConstantBuffer* cbv = std::get<1>(tuple);
			ShadowInfo* si = std::get<2>(tuple);
			m_pViewPool->ClearSpecificLight(type, cbv, si);

			// Remove from CopyPerFrame
			CopyPerFrameTask* cpft = nullptr;
			cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
			cpft->ClearSpecific(cbv->GetUploadResource());

			// Finally remove from m_pRenderer
			ShadowRenderTask* srt = static_cast<ShadowRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::SHADOW]);
			srt->ClearSpecificLight(std::get<0>(tuple));
			m_Lights[type].erase(m_Lights[type].begin() + count);

			// Update cbPerScene
			submitUploadPerSceneData();
			break;
		}
		count++;
	}
}

void Renderer::UnInitCameraComponent(component::CameraComponent* component)
{
}

void Renderer::UnInitBoundingBoxComponent(component::BoundingBoxComponent* component)
{
	// Check if the entity got a boundingbox component.
	if (component != nullptr)
	{
		if (component->GetParent() != nullptr)
		{
			// Stop drawing the wireFrame
			if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
			{
				static_cast<WireframeRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME])->ClearSpecific(component);
			}

			// Stop picking this boundingBox
			unsigned int i = 0;
			for (auto& bbcToBePicked : m_BoundingBoxesToBePicked)
			{
				if (bbcToBePicked == component)
				{
					m_BoundingBoxesToBePicked.erase(m_BoundingBoxesToBePicked.begin() + i);
					break;
				}
				i++;
			}
		}
	}
}

void Renderer::OnResetScene()
{
	// Lights will be cleared in respective UninitComponent function

	// Clear the rest
	m_RenderComponents.clear();
	m_pViewPool->ClearAll();
	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]->Clear();
	m_pScenePrimaryCamera = nullptr;
	static_cast<WireframeRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME])->Clear();
	m_BoundingBoxesToBePicked.clear();
}

void Renderer::submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->Submit(Upload_Default_Data);
}

void Renderer::submitMeshToCodt(Mesh* mesh)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);

	std::tuple<Resource*, Resource*, const void*> Vert_Upload_Default_Data(mesh->m_pUploadResourceVertices, mesh->m_pDefaultResourceVertices, mesh->m_Vertices.data());
	std::tuple<Resource*, Resource*, const void*> Indi_Upload_Default_Data(mesh->m_pUploadResourceIndices, mesh->m_pDefaultResourceIndices, mesh->m_Indices.data());

	codt->Submit(&Vert_Upload_Default_Data);
	codt->Submit(&Indi_Upload_Default_Data);
}

void Renderer::submitModelToGPU(Model* model)
{
	// Dont submit if already on GPU
	if (AssetLoader::Get()->IsModelLoadedOnGpu(model) == true)
	{
		return;
	}

	for (unsigned int i = 0; i < model->GetSize(); i++)
	{
		Mesh* mesh = model->GetMeshAt(i);

		// Submit Mesh
		submitMeshToCodt(mesh);

		Texture* texture;
		// Submit Material
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::ALBEDO);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::ROUGHNESS);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::METALLIC);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::NORMAL);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::EMISSIVE);
		submitTextureToCodt(texture);
		texture = model->GetMaterialAt(i)->GetTexture(TEXTURE2D_TYPE::OPACITY);
		submitTextureToCodt(texture);
	}

	AssetLoader::Get()->m_LoadedModels.at(model->GetPath()).first = true;
}

void Renderer::submitTextureToCodt(Texture* texture)
{
	if (AssetLoader::Get()->IsTextureLoadedOnGpu(texture) == true)
	{
		return;
	}

	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->SubmitTexture(texture);

	AssetLoader::Get()->m_LoadedTextures.at(texture->GetPath()).first = true;
}

void Renderer::submitToCpft(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->Submit(Upload_Default_Data);
}

void Renderer::clearSpecificCpft(Resource* upload)
{
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);
	cpft->ClearSpecific(upload);
}

DescriptorHeap* Renderer::getCBVSRVUAVdHeap() const
{
	return m_DescriptorHeaps.at(DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);
}

Entity* const Renderer::GetPickedEntity() const
{
	return m_pPickedEntity;
}

Scene* const Renderer::GetActiveScene() const
{
	return m_pCurrActiveScene;
}

Window* const Renderer::GetWindow() const
{
	return m_pWindow;
}

void Renderer::setRenderTasksPrimaryCamera()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_CONSTANT]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_TEXTURE]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::SHADOW]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[RENDER_TASK_TYPE::OUTLINE]->SetCamera(m_pScenePrimaryCamera);

	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME]->SetCamera(m_pScenePrimaryCamera);
	}
}

bool Renderer::createDevice()
{
	bool deviceCreated = false;

#ifdef _DEBUG
	//Enable the D3D12 debug layer.
	ID3D12Debug* debugController = nullptr;

#ifdef STATIC_LINK_DEBUGSTUFF
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SafeRelease(debugController);
#else
	HMODULE mD3D12 = LoadLibrary(L"D3D12.dll"); // ist�llet f�r GetModuleHandle

	PFN_D3D12_GET_DEBUG_INTERFACE f = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
	if (SUCCEEDED(f(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	SAFE_RELEASE(&debugController);
#endif
#endif

	IDXGIFactory6* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;

	CreateDXGIFactory(IID_PPV_ARGS(&factory));

	for (unsigned int adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			break; // No more adapters
		}
	
		// Check to see if the adapter supports Direct3D 12, but don't create the actual m_pDevice yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device5), nullptr)))
		{
			break;
		}
	
		SAFE_RELEASE(&adapter);
	}
	
	if (adapter)
	{
		HRESULT hr = S_OK;
		//Create the actual device.
		if (SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice5))))
		{
			deviceCreated = true;
		}
		else
		{
			Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to create Device\n");
		}
	
		SAFE_RELEASE(&adapter);
	}
	else
	{
		//Create warp m_pDevice if no adapter was found.
		factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice5));
	}

	SAFE_RELEASE(&factory);

	return deviceCreated;
}

void Renderer::createCommandQueues()
{
	// Direct
	D3D12_COMMAND_QUEUE_DESC cqdDirect = {};
	cqdDirect.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hr;
	hr = m_pDevice5->CreateCommandQueue(&cqdDirect, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]));
	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to Create Direct CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->SetName(L"DirectQueue");

	// Compute
	D3D12_COMMAND_QUEUE_DESC cqdCompute = {};
	cqdCompute.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = m_pDevice5->CreateCommandQueue(&cqdCompute, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]));
	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to Create Compute CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::COMPUTE_TYPE]->SetName(L"ComputeQueue");

	// Copy
	D3D12_COMMAND_QUEUE_DESC cqdCopy = {};
	cqdCopy.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_pDevice5->CreateCommandQueue(&cqdCopy, IID_PPV_ARGS(&m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]));
	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to Create Copy CommandQueue\n");
	}
	m_CommandQueues[COMMAND_INTERFACE_TYPE::COPY_TYPE]->SetName(L"CopyQueue");
}

void Renderer::createSwapChain()
{
	UINT resolutionWidth = 800;
	UINT resolutionHeight = 600;

	m_pSwapChain = new SwapChain(
		m_pDevice5,
		m_pWindow->GetHwnd(),
		resolutionWidth, resolutionHeight,
		m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
}

void Renderer::createMainDSV()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	UINT resolutionWidth = 0;
	UINT resolutionHeight = 0;
	m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&resolutionWidth, &resolutionHeight);

	m_pMainDepthStencil = new DepthStencil(
		m_pDevice5,
		resolutionWidth, resolutionHeight,
		L"MainDSV",
		&dsvDesc,
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV]);
}

void Renderer::createRootSignature()
{
	m_pRootSignature = new RootSignature(m_pDevice5);
}

void Renderer::createFullScreenQuad()
{
	std::vector<Vertex> vertexVector;
	std::vector<unsigned int> indexVector;

	Vertex vertices[4] = {};
	vertices[0].pos = { -1.0f, 1.0f, 1.0f };
	vertices[0].uv = { 0.0f, 0.0f, };

	vertices[1].pos = { -1.0f, -1.0f, 1.0f };
	vertices[1].uv = { 0.0f, 1.0f };

	vertices[2].pos = { 1.0f, 1.0f, 1.0f };
	vertices[2].uv = { 1.0f, 0.0f };

	vertices[3].pos = { 1.0f, -1.0f, 1.0f };
	vertices[3].uv = { 1.0f, 1.0f};

	for (unsigned int i = 0; i < 4; i++)
	{
		vertexVector.push_back(vertices[i]);
	}
	indexVector.push_back(1);
	indexVector.push_back(0);
	indexVector.push_back(3);
	indexVector.push_back(0);
	indexVector.push_back(2);
	indexVector.push_back(3);

	m_pFullScreenQuad = new Mesh(&vertexVector, &indexVector);

	// init dx12 resources
	m_pFullScreenQuad->Init(m_pDevice5, m_DescriptorHeaps.at(DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV));
}

void Renderer::updateMousePicker()
{
	m_pMousePicker->UpdateRay();

	component::BoundingBoxComponent* pickedBoundingBox = nullptr;

	float tempDist;
	float closestDist = MAXNUMBER;

	for (component::BoundingBoxComponent* bbc : m_BoundingBoxesToBePicked)
	{
		// Reset picked m_Entities from last frame
		bbc->IsPickedThisFrame() = false;

		for (unsigned int i = 0; i < bbc->GetNumBoundingBoxes(); i++)
		{
			if (m_pMousePicker->Pick(bbc, tempDist, i) == true)
			{
				if (tempDist < closestDist)
				{
					pickedBoundingBox = bbc;

					closestDist = tempDist;
				}
			}
		}
	}

	// If an object intersected with the ray
	if (closestDist < MAXNUMBER)
	{
		pickedBoundingBox->IsPickedThisFrame() = true;

		// Set the object to me drawn in outliningRenderTask
		Entity* parentOfPickedObject = pickedBoundingBox->GetParent();
		component::ModelComponent*		mc = parentOfPickedObject->GetComponent<component::ModelComponent>();
		component::TransformComponent*	tc = parentOfPickedObject->GetComponent<component::TransformComponent>();

		static_cast<OutliningRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::OUTLINE])->SetObjectToOutline(&std::make_pair(mc, tc));

		m_pPickedEntity = parentOfPickedObject;
	}
	else
	{
		// No object was picked, reset the outlingRenderTask
		static_cast<OutliningRenderTask*>(m_RenderTasks[RENDER_TASK_TYPE::OUTLINE])->Clear();
		m_pPickedEntity = nullptr;
	}
}

void Renderer::initRenderTasks()
{

#pragma region DepthPrePass

	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDepthPrePass = {};
	gpsdDepthPrePass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget
	gpsdDepthPrePass.NumRenderTargets = 0;
	gpsdDepthPrePass.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	// Depthstencil usage
	gpsdDepthPrePass.SampleDesc.Count = 1;
	gpsdDepthPrePass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDepthPrePass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDepthPrePass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDepthPrePass.RasterizerState.DepthBias = 0;
	gpsdDepthPrePass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdDepthPrePass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdDepthPrePass.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC depthPrePassRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDepthPrePass.BlendState.RenderTarget[i] = depthPrePassRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC depthPrePassDsd = {};
	depthPrePassDsd.DepthEnable = true;
	depthPrePassDsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthPrePassDsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// DepthStencil
	depthPrePassDsd.StencilEnable = false;
	gpsdDepthPrePass.DepthStencilState = depthPrePassDsd;
	gpsdDepthPrePass.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDepthPrePassVector;
	gpsdDepthPrePassVector.push_back(&gpsdDepthPrePass);

	RenderTask* DepthPrePassRenderTask = new DepthRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"DepthVertex.hlsl", L"DepthPixel.hlsl",
		&gpsdDepthPrePassVector,
		L"DepthPrePassPSO",
		FLAG_THREAD::RENDER);

	DepthPrePassRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	DepthPrePassRenderTask->SetSwapChain(m_pSwapChain);
	DepthPrePassRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion DepthPrePass

#pragma region ForwardRendering
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdForwardRender = {};
	gpsdForwardRender.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdForwardRender.NumRenderTargets = 2;
	gpsdForwardRender.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdForwardRender.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdForwardRender.SampleDesc.Count = 1;
	gpsdForwardRender.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdForwardRender.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdForwardRender.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdForwardRender.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdForwardRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = false;
	gpsdForwardRender.DepthStencilState = dsd;
	gpsdForwardRender.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	/* Forward rendering with stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdForwardRenderStencilTest = gpsdForwardRender;

	// Only change stencil testing
	dsd = {};
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0x00;
	dsd.StencilWriteMask = 0xff;
	const D3D12_DEPTH_STENCILOP_DESC stencilWriteAllways =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE,
		D3D12_COMPARISON_FUNC_ALWAYS
	};
	dsd.FrontFace = stencilWriteAllways;
	dsd.BackFace = stencilWriteAllways;

	gpsdForwardRenderStencilTest.DepthStencilState = dsd;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdForwardRenderVector;
	gpsdForwardRenderVector.push_back(&gpsdForwardRender);
	gpsdForwardRenderVector.push_back(&gpsdForwardRenderStencilTest);

	RenderTask* forwardRenderTask = new FowardRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"ForwardVertex.hlsl", L"ForwardPixel.hlsl",
		&gpsdForwardRenderVector,
		L"ForwardRenderingPSO",
		FLAG_THREAD::RENDER);

	forwardRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	forwardRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	forwardRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	forwardRenderTask->SetSwapChain(m_pSwapChain);
	forwardRenderTask->AddRenderTargetView("brightTarget", std::get<1>(*m_pBloomResources->GetBrightTuple()));
	forwardRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion ForwardRendering

#pragma region DownSampleTextureTask
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDownSampleTexture = {};
	gpsdDownSampleTexture.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdDownSampleTexture.NumRenderTargets = 1;
	gpsdDownSampleTexture.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

	// Depthstencil usage
	gpsdDownSampleTexture.SampleDesc.Count = 1;
	gpsdDownSampleTexture.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDownSampleTexture.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDownSampleTexture.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDownSampleTexture.RasterizerState.FrontCounterClockwise = false;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDownSampleTexture.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	dsd = {};
	dsd.DepthEnable = false;
	dsd.StencilEnable = false;
	gpsdDownSampleTexture.DepthStencilState = dsd;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDownSampleTextureVector;
	gpsdDownSampleTextureVector.push_back(&gpsdDownSampleTexture);

	RenderTask* downSampleTask = new DownSampleRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"DownSampleVertex.hlsl", L"DownSamplePixel.hlsl",
		&gpsdDownSampleTextureVector,
		L"DownSampleTexturePSO",
		std::get<2>(*m_pBloomResources->GetBrightTuple()),		// Read from this in actual resolution
		m_pBloomResources->GetPingPongResource(0)->GetRTV(),	// Write to this in 1280x720
		FLAG_THREAD::RENDER);
	
	static_cast<DownSampleRenderTask*>(downSampleTask)->SetFullScreenQuad(m_pFullScreenQuad);
	downSampleTask->SetSwapChain(m_pSwapChain);
	downSampleTask->SetDescriptorHeaps(m_DescriptorHeaps);
	static_cast<DownSampleRenderTask*>(downSampleTask)->SetFullScreenQuadInSlotInfo();

#pragma endregion DownSampleTextureTask

#pragma region ModelOutlining
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdModelOutlining = gpsdForwardRenderStencilTest;
	gpsdModelOutlining.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	dsd = {};
	dsd.DepthEnable = true;	// Maybe enable if we dont want the object to "highlight" through other objects
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	// DepthStencil
	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xff;
	dsd.StencilWriteMask = 0x00;
	const D3D12_DEPTH_STENCILOP_DESC stencilNotEqual =
	{ 
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_REPLACE,
		D3D12_COMPARISON_FUNC_NOT_EQUAL
	};
	dsd.FrontFace = stencilNotEqual;
	dsd.BackFace = stencilNotEqual;

	gpsdModelOutlining.DepthStencilState = dsd;
	gpsdModelOutlining.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdOutliningVector;
	gpsdOutliningVector.push_back(&gpsdModelOutlining);

	RenderTask* outliningRenderTask = new OutliningRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"OutlinedVertex.hlsl", L"OutlinedPixel.hlsl",
		&gpsdOutliningVector,
		L"outliningScaledPSO",
		FLAG_THREAD::RENDER);
	
	outliningRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	outliningRenderTask->SetSwapChain(m_pSwapChain);
	outliningRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion ModelOutlining

#pragma region Blend
	// ------------------------ TASK 2: BLEND ---------------------------- FRONTCULL

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendFrontCull = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendBackCull = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdParticleEffect = {};
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdBlendVector;
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdParticleVector;

	gpsdBlendFrontCull.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdBlendFrontCull.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdBlendFrontCull.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdBlendFrontCull.SampleDesc.Count = 1;
	gpsdBlendFrontCull.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdBlendFrontCull.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdBlendFrontCull.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC blendRTdesc{};
	blendRTdesc.BlendEnable = true;
	blendRTdesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendRTdesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendRTdesc.BlendOp = D3D12_BLEND_OP_ADD;
	blendRTdesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendRTdesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendRTdesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendRTdesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	{
		gpsdBlendFrontCull.BlendState.RenderTarget[i] = blendRTdesc;
	}

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsdBlend = {};
	dsdBlend.DepthEnable = true;
	dsdBlend.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsdBlend.DepthFunc = D3D12_COMPARISON_FUNC_LESS;	// Om pixels depth är lägre än den gamla så ritas den nya ut

	// DepthStencil
	dsdBlend.StencilEnable = false;
	dsdBlend.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	dsdBlend.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC blendStencilOP{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
	dsdBlend.FrontFace = blendStencilOP;
	dsdBlend.BackFace = blendStencilOP;

	gpsdBlendFrontCull.DepthStencilState = dsdBlend;
	gpsdBlendFrontCull.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	// ------------------------ TASK 2: BLEND ---------------------------- BACKCULL

	gpsdBlendBackCull.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdBlendBackCull.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdBlendBackCull.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdBlendBackCull.SampleDesc.Count = 1;
	gpsdBlendBackCull.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdBlendBackCull.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdBlendBackCull.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdBlendBackCull.BlendState.RenderTarget[i] = blendRTdesc;

	// DepthStencil
	dsdBlend.StencilEnable = false;
	dsdBlend.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;


	// Particle Effect
	gpsdParticleEffect.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdParticleEffect.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdParticleEffect.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdParticleEffect.SampleDesc.Count = 1;
	gpsdParticleEffect.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdParticleEffect.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdParticleEffect.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdParticleEffect.BlendState.RenderTarget[i] = blendRTdesc;

	// DepthStencil
	dsdBlend.StencilEnable = false;
	dsdBlend.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	gpsdBlendBackCull.DepthStencilState = dsdBlend;
	gpsdBlendBackCull.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	gpsdParticleEffect.DepthStencilState = dsdBlend;
	gpsdParticleEffect.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();


	// Push back to vector
	gpsdBlendVector.push_back(&gpsdBlendFrontCull);
	gpsdBlendVector.push_back(&gpsdBlendBackCull);


	gpsdParticleVector.push_back(&gpsdParticleEffect);

	RenderTask* transparentConstantRenderTask = new TransparentRenderTask(m_pDevice5,
		m_pRootSignature,
		L"TransparentConstantVertex.hlsl",
		L"TransparentConstantPixel.hlsl",
		&gpsdBlendVector,
		L"BlendPSOConstant",
		FLAG_THREAD::RENDER);

	transparentConstantRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	transparentConstantRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	transparentConstantRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	transparentConstantRenderTask->SetSwapChain(m_pSwapChain);
	transparentConstantRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
	
	/*---------------------------------- TRANSPARENT_TEXTURE_RENDERTASK -------------------------------------*/
	RenderTask* transparentTextureRenderTask = new TransparentRenderTask(m_pDevice5,
		m_pRootSignature,
		L"TransparentTextureVertex.hlsl",
		L"TransparentTexturePixel.hlsl",
		&gpsdBlendVector,
		L"BlendPSOTexture",
		FLAG_THREAD::RENDER);

	transparentTextureRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	transparentTextureRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	transparentTextureRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	transparentTextureRenderTask->SetSwapChain(m_pSwapChain);
	transparentTextureRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);

#pragma endregion Blend

#pragma region ShadowPass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdShadow = { 0 };
	gpsdShadow.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdShadow.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	gpsdShadow.NumRenderTargets = 0;
	// Depthstencil usage
	gpsdShadow.SampleDesc.Count = 1;
	gpsdShadow.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdShadow.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdShadow.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdShadow.RasterizerState.DepthBias = 1000;
	gpsdShadow.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdShadow.RasterizerState.SlopeScaledDepthBias = 3.0f;
	gpsdShadow.RasterizerState.FrontCounterClockwise = false;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsdShadow = {};
	dsdShadow.DepthEnable = true;
	dsdShadow.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsdShadow.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	dsdShadow.StencilEnable = false;

	gpsdShadow.DepthStencilState = dsdShadow;
	gpsdShadow.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultShadowDesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdShadow.BlendState.RenderTarget[i] = defaultShadowDesc;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdShadowVector;
	gpsdShadowVector.push_back(&gpsdShadow);

	RenderTask* shadowRenderTask = new ShadowRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"DepthVertex.hlsl", L"DepthPixel.hlsl",
		&gpsdShadowVector,
		L"ShadowPSO",
		FLAG_THREAD::RENDER);

	shadowRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
#pragma endregion ShadowPass

#pragma region WireFrame
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdWireFrame = {};
	gpsdWireFrame.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdWireFrame.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdWireFrame.NumRenderTargets = 1;
	// Depthstencil usage
	gpsdWireFrame.SampleDesc.Count = 1;
	gpsdWireFrame.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdWireFrame.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	gpsdWireFrame.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	gpsdWireFrame.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdWireFrame.BlendState.RenderTarget[i] = defaultRTdesc;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdWireFrameVector;
	gpsdWireFrameVector.push_back(&gpsdWireFrame);

	RenderTask* wireFrameRenderTask = new WireframeRenderTask(m_pDevice5,
		m_pRootSignature,
		L"WhiteVertex.hlsl", L"WhitePixel.hlsl",
		&gpsdWireFrameVector,
		L"WireFramePSO",
		FLAG_THREAD::RENDER);

	wireFrameRenderTask->SetSwapChain(m_pSwapChain);
	wireFrameRenderTask->SetDescriptorHeaps(m_DescriptorHeaps);
#pragma endregion WireFrame

#pragma region MergePass
	/* Forward rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdMergePass = {};
	gpsdMergePass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdMergePass.NumRenderTargets = 1;
	gpsdMergePass.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdMergePass.SampleDesc.Count = 1;
	gpsdMergePass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdMergePass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdMergePass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdMergePass.RasterizerState.FrontCounterClockwise = false;

	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdMergePass.BlendState.RenderTarget[i] = blendRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsdMergePass = {};
	dsdMergePass.DepthEnable = false;
	dsdMergePass.StencilEnable = false;
	gpsdMergePass.DepthStencilState = dsdMergePass;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdMergePassVector;
	gpsdMergePassVector.push_back(&gpsdMergePass);

	RenderTask* mergeTask = new MergeRenderTask(
		m_pDevice5,
		m_pRootSignature,
		L"MergeVertex.hlsl", L"MergePixel.hlsl",
		&gpsdMergePassVector,
		L"MergePassPSO",
		FLAG_THREAD::RENDER);

	static_cast<MergeRenderTask*>(mergeTask)->SetFullScreenQuad(m_pFullScreenQuad);
	static_cast<MergeRenderTask*>(mergeTask)->AddSRVIndexToMerge(m_pBloomResources->GetPingPongResource(0)->GetSRV()->GetDescriptorHeapIndex());
	mergeTask->SetSwapChain(m_pSwapChain);
	mergeTask->SetDescriptorHeaps(m_DescriptorHeaps);
	static_cast<MergeRenderTask*>(mergeTask)->CreateSlotInfo();
#pragma endregion MergePass


#pragma region ComputeAndCopyTasks
	UINT resolutionWidth = 0;
	UINT resolutionHeight = 0;
	m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&resolutionWidth, &resolutionHeight);

	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurHorizontal.hlsl", L"blurHorizontalPSO"));
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurVertical.hlsl", L"blurVerticalPSO"));
	ComputeTask* blurComputeTask = new BlurComputeTask(
		m_pDevice5, m_pRootSignature,
		csNamePSOName,
		COMMAND_INTERFACE_TYPE::DIRECT_TYPE,
		std::get<2>(*m_pBloomResources->GetBrightTuple()),
		m_pBloomResources->GetPingPongResource(0),
		m_pBloomResources->GetPingPongResource(1),
		m_pBloomResources->GetBlurWidth(), m_pBloomResources->GetBlurHeight(),
		FLAG_THREAD::RENDER);

	blurComputeTask->SetDescriptorHeaps(m_DescriptorHeaps);

	// CopyTasks
	CopyTask* copyPerFrameTask = new CopyPerFrameTask(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE, FLAG_THREAD::RENDER);
	CopyTask* copyOnDemandTask = new CopyOnDemandTask(m_pDevice5, COMMAND_INTERFACE_TYPE::DIRECT_TYPE, FLAG_THREAD::RENDER);

#pragma endregion ComputeAndCopyTasks
	// Add the tasks to desired vectors so they can be used in m_pRenderer
	/* -------------------------------------------------------------- */


	/* ------------------------- CopyQueue Tasks ------------------------ */

	m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME] = copyPerFrameTask;
	m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND] = copyOnDemandTask;

	/* ------------------------- ComputeQueue Tasks ------------------------ */
	
	m_ComputeTasks[COMPUTE_TASK_TYPE::BLUR] = blurComputeTask;

	/* ------------------------- DirectQueue Tasks ---------------------- */
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS] = DepthPrePassRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::SHADOW] = shadowRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER] = forwardRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_CONSTANT] = transparentConstantRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_TEXTURE] = transparentTextureRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::WIREFRAME] = wireFrameRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::OUTLINE] = outliningRenderTask;
	m_RenderTasks[RENDER_TASK_TYPE::MERGE] = mergeTask;
	m_RenderTasks[RENDER_TASK_TYPE::DOWNSAMPLE] = downSampleTask;

	// Pushback in the order of execution
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyOnDemandTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(copyPerFrameTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(DepthPrePassRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(shadowRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(forwardRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(downSampleTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(transparentConstantRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(transparentTextureRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(outliningRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			m_DirectCommandLists[i].push_back(wireFrameRenderTask->GetCommandInterface()->GetCommandList(i));
		}
	}

	// Compute shader to blur the RTV from forwardRenderTask
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(blurComputeTask->GetCommandInterface()->GetCommandList(i));
	}

	// Final pass (this pass will merge different textures together and put result in the swapchain backBuffer)
	// This will be used for pp-effects such as bloom.
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(mergeTask->GetCommandInterface()->GetCommandList(i));
	}
}

void Renderer::setRenderTasksRenderComponents()
{
	m_RenderTasks[RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetRenderComponents(&m_RenderComponents[FLAG_DRAW::NO_DEPTH]);
	m_RenderTasks[RENDER_TASK_TYPE::FORWARD_RENDER]->SetRenderComponents(&m_RenderComponents[FLAG_DRAW::DRAW_OPAQUE]);
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_CONSTANT]->SetRenderComponents(&m_RenderComponents[FLAG_DRAW::DRAW_TRANSPARENT_CONSTANT]);
	m_RenderTasks[RENDER_TASK_TYPE::TRANSPARENT_TEXTURE]->SetRenderComponents(&m_RenderComponents[FLAG_DRAW::DRAW_TRANSPARENT_TEXTURE]);
	m_RenderTasks[RENDER_TASK_TYPE::SHADOW]->SetRenderComponents(&m_RenderComponents[FLAG_DRAW::GIVE_SHADOW]);
}

void Renderer::createDescriptorHeaps()
{
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV] = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV] = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::RTV);
	m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::DSV] = new DescriptorHeap(m_pDevice5, DESCRIPTOR_HEAP_TYPE::DSV);
}

void Renderer::createFences()
{
	HRESULT hr = m_pDevice5->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFenceFrame));

	if (FAILED(hr))
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to Create Fence\n");
	}
	m_FenceFrameValue = 1;

	// Event handle to use for GPU synchronization
	m_EventHandle = CreateEvent(0, false, false, 0);
}

void Renderer::waitForGPU()
{
	//Signal and increment the fence value.
	const UINT64 oldFenceValue = m_FenceFrameValue;
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, oldFenceValue);
	m_FenceFrameValue++;

	//Wait until command queue is done.
	if (m_pFenceFrame->GetCompletedValue() < oldFenceValue)
	{
		m_pFenceFrame->SetEventOnCompletion(oldFenceValue, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

void Renderer::waitForFrame(unsigned int framesToBeAhead)
{
	static constexpr unsigned int nrOfFenceChangesPerFrame = 1;
	unsigned int fenceValuesToBeAhead = framesToBeAhead * nrOfFenceChangesPerFrame;

	//Wait if the CPU is "framesToBeAhead" number of frames ahead of the GPU
	if (m_pFenceFrame->GetCompletedValue() < m_FenceFrameValue - fenceValuesToBeAhead)
	{
		m_pFenceFrame->SetEventOnCompletion(m_FenceFrameValue - fenceValuesToBeAhead, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

void Renderer::prepareScene(Scene* activeScene)
{
	submitUploadPerFrameData();
	submitUploadPerSceneData();

	// -------------------- DEBUG STUFF --------------------
	// Test to change m_pCamera to the shadow casting m_lights cameras
	//auto& tuple = m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].at(0);
	//BaseCamera* tempCam = std::get<0>(tuple)->GetCamera();
	//m_pScenePrimaryCamera = tempCam;

	if (m_pScenePrimaryCamera == nullptr)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "No primary camera was set in scenes\n");

		// Todo: Set default m_pCamera
	}

	m_pMousePicker->SetPrimaryCamera(m_pScenePrimaryCamera);

	setRenderTasksRenderComponents();
	setRenderTasksPrimaryCamera();
}

void Renderer::submitUploadPerSceneData()
{
	*m_pCbPerSceneData = {};
	// ----- directional lights -----
	m_pCbPerSceneData->Num_Dir_Lights = m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].size();
	unsigned int index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT])
	{
		m_pCbPerSceneData->dirLightIndices[index].x = std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex();
		index++;
	}
	// ----- directional m_lights -----

	// ----- point lights -----
	m_pCbPerSceneData->Num_Point_Lights = m_Lights[LIGHT_TYPE::POINT_LIGHT].size();
	index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::POINT_LIGHT])
	{
		m_pCbPerSceneData->pointLightIndices[index].x = std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex();
		index++;
	}
	// ----- point m_lights -----

	// ----- spot lights -----
	m_pCbPerSceneData->Num_Spot_Lights = m_Lights[LIGHT_TYPE::SPOT_LIGHT].size();
	index = 0;
	for (auto& tuple : m_Lights[LIGHT_TYPE::SPOT_LIGHT])
	{
		m_pCbPerSceneData->spotLightIndices[index].x = std::get<1>(tuple)->GetCBV()->GetDescriptorHeapIndex();
		index++;
	}
	// ----- spot m_lights -----
	
	// Submit CB_PER_SCENE to be uploaded to VRAM
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(m_pCbPerSceneData);
	codt->Submit(&std::make_tuple(m_pCbPerScene->GetUploadResource(), m_pCbPerScene->GetDefaultResource(), data));
}

void Renderer::submitUploadPerFrameData()
{
	// Submit dynamic-light-data to be uploaded to VRAM
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[COPY_TASK_TYPE::COPY_PER_FRAME]);

	// CB_PER_FRAME_STRUCT
	if (cpft != nullptr)
	{
		const void* data = static_cast<void*>(m_pCbPerFrameData);
		cpft->Submit(&std::tuple(m_pCbPerFrame->GetUploadResource(), m_pCbPerFrame->GetDefaultResource(), data));
	}
}

void Renderer::toggleFullscreen(WindowChange* evnt)
{
	m_FenceFrameValue++;
	m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE]->Signal(m_pFenceFrame, m_FenceFrameValue);

	// Wait for all frames
	waitForFrame(0);

	// Wait for the threads which records the commandlists to complete
	m_pThreadPool->WaitForThreads(FLAG_THREAD::RENDER);

	for (auto task : m_RenderTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}
	for (auto task : m_CopyTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}
	for (auto task : m_ComputeTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->Reset(i);
		}
	}

	m_pSwapChain->ToggleWindowMode(m_pDevice5,
		m_pWindow->GetHwnd(),
		m_CommandQueues[COMMAND_INTERFACE_TYPE::DIRECT_TYPE],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::RTV],
		m_DescriptorHeaps[DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	// Change the member variables of the window class to match the swapchain
	UINT width = 0, height = 0;
	if (m_pSwapChain->IsFullscreen())
	{
		m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&width, &height);
	}
	else
	{
		// Earlier it read from options. now just set to 800/600
		width = 800;
		height = 600;
	}

	Window* window = const_cast<Window*>(m_pWindow);
	window->SetScreenWidth(width);
	window->SetScreenHeight(height);

	for (auto task : m_RenderTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
	for (auto task : m_CopyTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
	for (auto task : m_ComputeTasks)
	{
		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
		{
			task->GetCommandInterface()->GetCommandList(i)->Close();
		}
	}
}

SwapChain* Renderer::getSwapChain() const
{
	return m_pSwapChain;
}
