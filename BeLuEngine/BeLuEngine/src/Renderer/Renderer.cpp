#include "stdafx.h"
#include "Renderer.h"

// Misc
#include "../Misc/AssetLoader.h"
#include "../Misc/Log.h"
#include "../Misc/MultiThreading/ThreadPool.h"
#include "../Misc/MultiThreading/Thread.h"
#include "../Misc/Window.h"
#include "DXILShaderCompiler.h"

// Headers
#include "Core.h"

// Debug
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"
#include "../ImGui/ImGuiHandler.h"
#include "Statistics/EngineStatistics.h"
#include "DX12Tasks/ImGuiRenderTask.h"

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
#include "SwapChain.h"
#include "CommandInterface.h"
#include "DescriptorHeap.h"
#include "Geometry/Transform.h"
#include "Camera/BaseCamera.h"
#include "Geometry/Model.h"
#include "Geometry/Mesh.h"
#include "Texture/Texture.h"
#include "Texture/Texture2D.h"
#include "Texture/Texture2DGUI.h"
#include "Texture/TextureCubeMap.h"
#include "Geometry/Material.h"

#include "GPUMemory/GPUMemory.h"

// Techniques
#include "Techniques/Bloom.h"
#include "Techniques/PingPongResource.h"
#include "Techniques/ShadowInfo.h"
#include "Techniques/MousePicker.h"
#include "Techniques/BoundingBoxPool.h"

// Graphics
#include "DX12Tasks/DepthRenderTask.h"
#include "DX12Tasks/WireframeRenderTask.h"
#include "DX12Tasks/OutliningRenderTask.h"
#include "DX12Tasks/DeferredGeometryRenderTask.h"
#include "DX12Tasks/DeferredLightRenderTask.h"
#include "DX12Tasks/TransparentRenderTask.h"
#include "DX12Tasks/DownSampleRenderTask.h"
#include "DX12Tasks/MergeRenderTask.h"

// Copy 
#include "DX12Tasks/CopyPerFrameTask.h"
#include "DX12Tasks/CopyPerFrameMatricesTask.h"
#include "DX12Tasks/CopyOnDemandTask.h"

// Compute
#include "DX12Tasks/BlurComputeTask.h"

// DXR
#include "DX12Tasks/BottomLevelRenderTask.h"
#include "DX12Tasks/TopLevelRenderTask.h"
#include "DX12Tasks/DXRReflectionTask.h"
#include "DXR/BottomLevelAccelerationStructure.h"
#include "DXR/TopLevelAccelerationStructure.h"

// Event
#include "../Events/EventBus.h"


// ABSTRACTION TEMP
#include "API/D3D12/D3D12GraphicsManager.h"

Renderer::Renderer()
{
	//EventBus::GetInstance().Subscribe(this, &Renderer::toggleFullscreen);
	m_RenderTasks.resize(E_RENDER_TASK_TYPE::NR_OF_RENDERTASKS);
	m_CopyTasks.resize(E_COPY_TASK_TYPE::NR_OF_COPYTASKS);
	m_ComputeTasks.resize(E_COMPUTE_TASK_TYPE::NR_OF_COMPUTETASKS);
	m_DX12Tasks.resize(E_DX12_TASK_TYPE::NR_OF_DX12TASKS);
	m_DXRTasks.resize(E_DXR_TASK_TYPE::NR_OF_DXRTASKS);
	
	// Processinfo
	// Create handle to process
	DWORD currentProcessID = GetCurrentProcessId();
	m_ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, currentProcessID);

	if (m_ProcessHandle == nullptr)
	{
		BL_LOG_CRITICAL("Failed to create handle to process\n");
	}
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
	TODO("Fix this");
	//waitForGPU();

	delete m_pFullScreenQuad;

	delete m_FinalColorBuffer.first;
	delete m_FinalColorBuffer.second;

	delete m_GBufferAlbedo.first;
	delete m_GBufferAlbedo.second;
	delete m_GBufferNormal.first;
	delete m_GBufferNormal.second;
	delete m_GBufferMaterialProperties.first;
	delete m_GBufferMaterialProperties.second;
	delete m_GBufferEmissive.first;
	delete m_GBufferEmissive.second;

	delete m_ReflectionTexture.resource;
	delete m_ReflectionTexture.uav;
	delete m_ReflectionTexture.srv;

	delete m_pBloomResources;
	delete m_pMainDepthStencil;

	for (ComputeTask* computeTask : m_ComputeTasks)
		delete computeTask;

	for (CopyTask* copyTask : m_CopyTasks)
		delete copyTask;

	for (RenderTask* renderTask : m_RenderTasks)
		delete renderTask;

	for (DX12Task* dx12Task : m_DX12Tasks)
		delete dx12Task;

	for (DXRTask* dxrTask : m_DXRTasks)
		delete dxrTask;

	delete m_pMousePicker;

	delete m_pCbPerScene;
	delete m_pCbPerSceneData;
	delete m_pCbPerFrame;
	delete m_pCbPerFrameData;

	delete Light::m_pLightsRawBuffer;
	free(Light::m_pRawData);

#ifdef DEBUG
	// Cleanup ImGui
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

void Renderer::InitD3D12(HWND hwnd, unsigned int width, unsigned int height, HINSTANCE hInstance, ThreadPool* threadPool)
{
	m_pThreadPool = threadPool;

	m_CurrentRenderingWidth = width;
	m_CurrentRenderingHeight = height;

	// ABSTRACTION TEMP
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap= static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap= static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();

#pragma region RenderTargets
	// Main color renderTarget (used until the swapchain RT is drawn to)
	m_FinalColorBuffer.first = new RenderTarget(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"finalColorBuffer_RESOURCE",
		rtvHeap,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	m_FinalColorBuffer.second = new ShaderResourceView(
		m_pDevice5,
		mainHeap,
		m_FinalColorBuffer.first->GetDefaultResource());

	// GBufferAlbedo
	m_GBufferAlbedo.first = new RenderTarget(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"gBufferAlbedo_RESOURCE",
		rtvHeap,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	srvDesc = {};
	m_GBufferAlbedo.second = new ShaderResourceView(
		m_pDevice5,
		mainHeap,
		m_GBufferAlbedo.first->GetDefaultResource());

	// Normal
	m_GBufferNormal.first = new RenderTarget(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"gBufferNormal_RESOURCE",
		rtvHeap,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_GBufferNormal.second = new ShaderResourceView(
		m_pDevice5,
		mainHeap,
		m_GBufferNormal.first->GetDefaultResource());

	// Material Properties (Roughness, Metallic, glow..
	m_GBufferMaterialProperties.first = new RenderTarget(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"gBufferMaterials_RESOURCE",
		rtvHeap,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_GBufferMaterialProperties.second = new ShaderResourceView(
		m_pDevice5,
		mainHeap,
		m_GBufferMaterialProperties.first->GetDefaultResource());

	// Emissive Color
	m_GBufferEmissive.first = new RenderTarget(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"gBufferEmissive_RESOURCE",
		rtvHeap,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	m_GBufferEmissive.second = new ShaderResourceView(
		m_pDevice5,
		mainHeap,
		m_GBufferEmissive.first->GetDefaultResource());

#pragma region ReflectionTexture
	{
		// Resource
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		resourceDesc.Width = m_CurrentRenderingWidth;
		resourceDesc.Height = m_CurrentRenderingHeight;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

		m_ReflectionTexture.resource = new Resource(m_pDevice5, &resourceDesc, nullptr, L"ReflectionTexture_RESOURCE", D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// UAV
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDescReflection = {};
		uavDescReflection.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		uavDescReflection.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDescReflection.Texture2D.MipSlice = 0;
		uavDescReflection.Texture2D.PlaneSlice = 0;

		m_ReflectionTexture.uav = new UnorderedAccessView(
			m_pDevice5,
			mainHeap,
			&uavDescReflection,
			m_ReflectionTexture.resource);

		// SRV
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDescReflection = {};
		srvDescReflection.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDescReflection.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		srvDescReflection.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDescReflection.Texture2D.MipLevels = -1;

		m_ReflectionTexture.srv = new ShaderResourceView(
			m_pDevice5,
			mainHeap,
			&srvDescReflection,
			m_ReflectionTexture.resource);
	}
#pragma endregion

	// Bloom
	m_pBloomResources = new Bloom(m_pDevice5, rtvHeap,mainHeap);

	// Create Main DepthBuffer
	createMainDSV();
#pragma endregion RenderTargets

	// Picking
	m_pMousePicker = new MousePicker();
	
	DXILShaderCompiler::Get()->Init();

	// FullScreenQuad
	createFullScreenQuad();

	// Init Assetloader
	AssetLoader* al = AssetLoader::Get(m_pDevice5, mainHeap);

	// Init BoundingBoxPool
	BoundingBoxPool::Get(m_pDevice5, mainHeap);

	// Allocate memory for cbPerScene
	m_pCbPerScene = new ConstantBuffer(
		m_pDevice5, 
		sizeof(CB_PER_SCENE_STRUCT),
		L"CB_PER_SCENE",
		mainHeap);
	
	m_pCbPerSceneData = new CB_PER_SCENE_STRUCT();

	// Allocate memory for cbPerFrame
	m_pCbPerFrame = new ConstantBuffer(
		m_pDevice5,
		sizeof(CB_PER_FRAME_STRUCT),
		L"CB_PER_FRAME",
		mainHeap);

	m_pCbPerFrameData = new CB_PER_FRAME_STRUCT();

#ifdef DEBUG
	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup ImGui style
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouse;

	io.DisplaySize.x = m_CurrentRenderingWidth;
	io.DisplaySize.y = m_CurrentRenderingHeight;

	unsigned int imGuiTextureIndex = mainHeap->GetNextDescriptorHeapIndex(1);

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(m_pDevice5, NUM_SWAP_BUFFERS,
		DXGI_FORMAT_R16G16B16A16_FLOAT, mainHeap->GetID3D12DescriptorHeap(),
		mainHeap->GetCPUHeapAt(imGuiTextureIndex),
		mainHeap->GetGPUHeapAt(imGuiTextureIndex));
#endif
	createRawBufferForLights();

	initRenderTasks();

	submitMeshToCodt(m_pFullScreenQuad);
}

void Renderer::Update(double dt)
{
	float3 right = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetRightVector());
	right.normalize();

	float3 forward = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetDirection());
	forward.normalize();

	// TODO: fix camera up vector
	float3 up = forward.cross(right);
	up.normalize();

	// Update CB_PER_FRAME data
	m_pCbPerFrameData->camPos = reinterpret_cast<float3&>(m_pScenePrimaryCamera->GetPosition());
	m_pCbPerFrameData->camRight = right;
	m_pCbPerFrameData->camUp = up;
	m_pCbPerFrameData->camForward = forward;

	m_pCbPerFrameData->projection	= *m_pScenePrimaryCamera->GetProjMatrix();
	m_pCbPerFrameData->projectionI	= *m_pScenePrimaryCamera->GetProjMatrixInverse();
	m_pCbPerFrameData->view			= *m_pScenePrimaryCamera->GetViewMatrix();
	m_pCbPerFrameData->viewI		= *m_pScenePrimaryCamera->GetViewMatrixInverse();

	// Picking
	updateMousePicker();

	// ImGui
#ifdef DEBUG
	ImGuiHandler::GetInstance().NewFrame();
#endif

	// DXR
	TopLevelAccelerationStructure* pTLAS = static_cast<TopLevelRenderTask*>(m_DX12Tasks[E_DX12_TASK_TYPE::TLAS])->GetTLAS();

	pTLAS->Reset();

	unsigned int i = 0;
	for (RenderComponent rc : m_RayTracedRenderComponents)
	{
		pTLAS->AddInstance(rc.mc->GetModel()->m_pBLAS, *rc.tc->GetTransform()->GetWorldMatrix(), i);
		i++;
	}

	// ABSTRACTION TEMP
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	pTLAS->SetupAccelerationStructureForBuilding(m_pDevice5, true);
}

void Renderer::SortObjects()
{
	struct DistFromCamera
	{
		double distance = 0.0f;
		RenderComponent rc = {nullptr, nullptr};
	};

	// Sort all vectors
	for (auto& renderComponents : m_RenderComponents)
	{
		unsigned int numRenderComponents = static_cast<unsigned int>(renderComponents.second.size());

		// Not the best practice to allocate on the heap every frame
		DistFromCamera* distFromCamArr = new DistFromCamera[numRenderComponents];

		// Get all the distances of each objects and store them by ID and distance
		DirectX::XMFLOAT3 camPos = m_pScenePrimaryCamera->GetPosition();
		for (unsigned int i = 0; i < numRenderComponents; i++)
		{
			DirectX::XMFLOAT3 objectPos = renderComponents.second.at(i).tc->GetTransform()->GetPositionXMFLOAT3();

			double distance = sqrt(pow(camPos.x - objectPos.x, 2) +
									pow(camPos.y - objectPos.y, 2) +
									pow(camPos.z - objectPos.z, 2));

			// Save the object alongside its distance to the m_pCamera
			distFromCamArr[i].distance = distance;
			distFromCamArr[i].rc = renderComponents.second.at(i);

		}

		// InsertionSort (because its best case is O(N)), 
		// and since this is sorted ((((((EVERY FRAME)))))) this is a good choice of sorting algorithm
		unsigned int j = 0;
		DistFromCamera distFromCamArrTemp = {};
		for (unsigned int i = 1; i < numRenderComponents; i++)
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
		for (unsigned int i = 0; i < numRenderComponents; i++)
		{
			renderComponents.second.emplace_back(distFromCamArr[i].rc.mc, distFromCamArr[i].rc.tc);
		}

		// Free memory
		delete[] distFromCamArr;
	}

	// Update the entity-arrays inside the rendertasks
	setRenderTasksRenderComponents();
}

void Renderer::ExecuteMT()
{
	IGraphicsManager* graphicsManager = IGraphicsManager::GetBaseInstance();
	graphicsManager->Begin();

	unsigned int commandInterfaceIndex = m_FrameCounter++ % NUM_SWAP_BUFFERS;

	DX12Task::SetBackBufferIndex(commandInterfaceIndex);
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	CopyTask* copyTask = nullptr;
	ComputeTask* computeTask = nullptr;
	RenderTask* renderTask = nullptr;
	DX12Task* dx12Task = nullptr;
	DXRTask* dxrTask = nullptr;
	/* --------------------- Record command lists --------------------- */

	// Copy on demand
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND];
	m_pThreadPool->AddTask(copyTask);

	// Copy per frame
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME];
	m_pThreadPool->AddTask(copyTask);

	// Copy per frame (matrices, world and wvp)
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME_MATRICES];
	static_cast<CopyPerFrameMatricesTask*>(copyTask)->SetCamera(m_pScenePrimaryCamera);
	m_pThreadPool->AddTask(copyTask);

	// Update BLASes if any...
	dx12Task = m_DX12Tasks[E_DX12_TASK_TYPE::BLAS];
	m_pThreadPool->AddTask(dx12Task);

	// Depth pre-pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEPTH_PRE_PASS];
	m_pThreadPool->AddTask(renderTask);

	// Update TLAS
	dx12Task = m_DX12Tasks[E_DX12_TASK_TYPE::TLAS];
	m_pThreadPool->AddTask(dx12Task);

	// Geometry pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY];
	renderTask->Execute();

	// Light pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_LIGHT];
	renderTask->Execute();

	// DXR Reflections
	dxrTask = m_DXRTasks[E_DXR_TASK_TYPE::REFLECTIONS];
	m_pThreadPool->AddTask(dxrTask);

	// DownSample the texture used for bloom
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DOWNSAMPLE];
	m_pThreadPool->AddTask(renderTask);

	// Blend
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::OPACITY];
	m_pThreadPool->AddTask(renderTask);

	// Blurring for bloom
	computeTask = m_ComputeTasks[E_COMPUTE_TASK_TYPE::BLUR];
	m_pThreadPool->AddTask(computeTask);

	// Outlining, if an object is picked
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE];
	m_pThreadPool->AddTask(renderTask);

	// Merge 
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::MERGE];
	m_pThreadPool->AddTask(renderTask);
	
	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME];
		m_pThreadPool->AddTask(renderTask);
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */

	// Wait for the threads which records the commandlists to complete
	m_pThreadPool->WaitForThreads(F_THREAD_FLAGS::RENDER);


	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_DirectCommandLists[commandInterfaceIndex], m_DirectCommandLists[commandInterfaceIndex].size());


	/* --------------------------------------------------------------- */

	// Clear copy on demand
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]->Clear();

	// ImGui
#ifdef DEBUG
	// Have to update ImGui here to get all information that happens inside rendering
	ImGuiHandler::GetInstance().UpdateFrame();

	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::IMGUI];
	renderTask->Execute();

	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_ImGuiCommandLists[commandInterfaceIndex], m_ImGuiCommandLists[commandInterfaceIndex].size());
#endif


	/*------------------- Present -------------------*/
	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->SyncAndPresent();
	
	// Check to end ImGui if its active
	ImGuiHandler::GetInstance().EndFrame();

	graphicsManager->End();
}

void Renderer::ExecuteST()
{
	IGraphicsManager* graphicsManager = IGraphicsManager::GetBaseInstance();
	graphicsManager->Begin();

	unsigned int commandInterfaceIndex = m_FrameCounter++ % NUM_SWAP_BUFFERS;

	DX12Task::SetBackBufferIndex(commandInterfaceIndex);
	DX12Task::SetCommandInterfaceIndex(commandInterfaceIndex);

	CopyTask* copyTask = nullptr;
	ComputeTask* computeTask = nullptr;
	RenderTask* renderTask = nullptr;
	DX12Task* dx12Task = nullptr;
	DXRTask* dxrTask = nullptr;

	/* --------------------- Record command lists --------------------- */
	// Copy on demand
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND];
	copyTask->Execute();

	// Copy per frame
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME];
	copyTask->Execute();

	// Copy per frame (matrices, world and wvp)
	copyTask = m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME_MATRICES];
	static_cast<CopyPerFrameMatricesTask*>(copyTask)->SetCamera(m_pScenePrimaryCamera);
	copyTask->Execute();

	// Update BLASes if any...
	dx12Task = m_DX12Tasks[E_DX12_TASK_TYPE::BLAS];
	dx12Task->Execute();

	// Depth pre-pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEPTH_PRE_PASS];
	renderTask->Execute();

	// Update TLAS
	dx12Task = m_DX12Tasks[E_DX12_TASK_TYPE::TLAS];
	dx12Task->Execute();

	// Geometry pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY];
	renderTask->Execute();

	// Light pass
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_LIGHT];
	renderTask->Execute();

	// DXR Reflections
	dxrTask = m_DXRTasks[E_DXR_TASK_TYPE::REFLECTIONS];
	dxrTask->Execute();

	// DownSample the texture used for bloom
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::DOWNSAMPLE];
	renderTask->Execute();

	// Blend
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::OPACITY];
	renderTask->Execute();

	// Blurring for bloom
	computeTask = m_ComputeTasks[E_COMPUTE_TASK_TYPE::BLUR];
	computeTask->Execute();

	// Outlining, if an object is picked
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE];
	renderTask->Execute();

	// Merge 
	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::MERGE];
	renderTask->Execute();

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME];
		renderTask->Execute();
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_DirectCommandLists[commandInterfaceIndex], m_DirectCommandLists[commandInterfaceIndex].size());

	/* --------------------------------------------------------------- */

	/*------------------- Post draw stuff -------------------*/
	// Clear copy on demand
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]->Clear();

	// ImGui
#ifdef DEBUG
	// Have to update ImGui here to get all information that happens inside rendering
	ImGuiHandler::GetInstance().UpdateFrame();

	renderTask = m_RenderTasks[E_RENDER_TASK_TYPE::IMGUI];
	renderTask->Execute();

	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_ImGuiCommandLists[commandInterfaceIndex], m_ImGuiCommandLists[commandInterfaceIndex].size());
#endif

	/*------------------- Present -------------------*/
	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->SyncAndPresent();

	// Check to end ImGui if its active
	ImGuiHandler::GetInstance().EndFrame();

	graphicsManager->End();
}

void Renderer::InitModelComponent(component::ModelComponent* mc)
{
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();

	component::TransformComponent* tc = mc->GetParent()->GetComponent<component::TransformComponent>();

	// Submit to codt
	submitModelToGPU(mc->m_pModel);

	mc->UpdateMaterialRawBufferFromMaterial();
	submitMaterialToGPU(mc);

	// Only add the m_Entities that actually should be drawn
	if (tc != nullptr)
	{
		Transform* t = tc->GetTransform();
		t->m_pCB = new ConstantBuffer(m_pDevice5, sizeof(DirectX::XMMATRIX) * 2, L"Transform", mainHeap);
		ConstantBuffer* cb = t->m_pCB;

		t->UpdateWorldMatrix();
		m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME_MATRICES]->Submit(&std::make_tuple(cb->GetUploadResource(), cb->GetDefaultResource(), static_cast<const void*>(t)));

		// Finally store the object in the corresponding renderComponent vectors so it will be drawn
		if (F_DRAW_FLAGS::DRAW_TRANSPARENT & mc->GetDrawFlag())
		{
			m_RenderComponents[F_DRAW_FLAGS::DRAW_TRANSPARENT].emplace_back(mc, tc);
		}

		if (F_DRAW_FLAGS::DRAW_OPAQUE & mc->GetDrawFlag())
		{
			m_RenderComponents[F_DRAW_FLAGS::DRAW_OPAQUE].emplace_back(mc, tc);
		}

		if (F_DRAW_FLAGS::NO_DEPTH & ~mc->GetDrawFlag())
		{
			m_RenderComponents[F_DRAW_FLAGS::NO_DEPTH].emplace_back(mc, tc);
		}

		if (F_DRAW_FLAGS::GIVE_SHADOW & mc->GetDrawFlag())
		{
			m_RenderComponents[F_DRAW_FLAGS::GIVE_SHADOW].emplace_back(mc, tc);
		}

		m_RayTracedRenderComponents.emplace_back(mc, tc);

		submitSlotInfoRawBufferToGPU(mc);
	}
}

void Renderer::InitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	unsigned int offset = sizeof(LightHeader);

	unsigned int numDirLights = *Light::m_pRawData;		// First unsigned int in memory is numDirLights

	offset += numDirLights * sizeof(DirectionalLight);
	static_cast<Light*>(component)->m_LightOffsetInArray = numDirLights;
	// Increase numLights by one, copy into buffer
	numDirLights += 1;

	memcpy(Light::m_pRawData, &numDirLights, sizeof(unsigned int));

	// Copy lightData
	memcpy(Light::m_pRawData + offset * sizeof(unsigned char), static_cast<DirectionalLight*>(component->GetLightData()), sizeof(DirectionalLight));
}

void Renderer::InitPointLightComponent(component::PointLightComponent* component)
{
	unsigned int offset = sizeof(LightHeader);

	unsigned int numPointLights = *(Light::m_pRawData + 4);// Second unsigned int in memory is numPointLights
	
	offset += DIR_LIGHT_MAXOFFSET;
	offset += numPointLights * sizeof(PointLight);

	static_cast<Light*>(component)->m_LightOffsetInArray = numPointLights;
	// Increase numLights by one, copy into buffer
	numPointLights += 1;

	memcpy(Light::m_pRawData + 4, &numPointLights, sizeof(unsigned int));

	// Copy lightData
	memcpy(Light::m_pRawData + offset * sizeof(unsigned char), static_cast<PointLight*>(component->GetLightData()), sizeof(PointLight));
}

void Renderer::InitSpotLightComponent(component::SpotLightComponent* component)
{
	unsigned int offset = sizeof(LightHeader);

	unsigned int numSpotLights = *(Light::m_pRawData + 8);// Third unsigned int in memory is numSpotLights

	offset += DIR_LIGHT_MAXOFFSET + POINT_LIGHT_MAXOFFSET;
	offset += numSpotLights * sizeof(SpotLight);

	static_cast<Light*>(component)->m_LightOffsetInArray = numSpotLights;
	// Increase numLights by one, copy into buffer
	numSpotLights += 1;

	memcpy(Light::m_pRawData + 8, &numSpotLights, sizeof(unsigned int));

	// Copy lightData
	memcpy(Light::m_pRawData + offset * sizeof(unsigned char), static_cast<SpotLight*>(component->GetLightData()), sizeof(SpotLight));
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
				BL_LOG_WARNING("Forgot to initialize BoundingBoxComponent on Entity: %s\n", component->GetParent()->GetName().c_str());
				return;
			}

			if (toBeSubmitted == true)
			{
				submitMeshToCodt(mesh);
			}

			component->AddMesh(mesh);
		}
		static_cast<WireframeRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME])->AddObjectToDraw(component);
	}

	// Add to vector so the mouse picker can check for intersections
	if (component->GetFlagOBB() & F_BOUNDING_BOX_FLAGS::PICKING)
	{
		m_BoundingBoxesToBePicked.push_back(component);
	}
}

void Renderer::UnInitModelComponent(component::ModelComponent* mc)
{
	// Remove component from renderComponents
	// TODO: change data structure to allow O(1) add and remove
	for (auto& renderComponent : m_RenderComponents)
	{
		std::vector<RenderComponent>& rcVec = renderComponent.second;
		for (int i = 0; i < rcVec.size(); i++)
		{
			// Remove from all renderComponent-vectors if they are there
			component::ModelComponent* comp = nullptr;
			comp = rcVec[i].mc;
			if (comp == mc)
			{
				rcVec.erase(renderComponent.second.begin() + i);
			}
		}
	}

	component::TransformComponent* tc = mc->GetParent()->GetComponent<component::TransformComponent>();

	// Unsubmit from updating to GPU every frame
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME_MATRICES]->ClearSpecific(tc->GetTransform()->m_pCB->GetUploadResource());

	// Delete constantBuffer for matrices
	if (tc->GetTransform()->m_pCB != nullptr)
	{
		delete tc->GetTransform()->m_pCB;
		tc->GetTransform()->m_pCB = nullptr;
	}

	// Update Render Tasks components (forward the change in renderComponents)
	setRenderTasksRenderComponents();
}

void Renderer::UnInitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	// TODO: Remove from rawBuffer
}

void Renderer::UnInitPointLightComponent(component::PointLightComponent* component)
{
	// TODO: Remove from rawBuffer
}

void Renderer::UnInitSpotLightComponent(component::SpotLightComponent* component)
{
	// TODO: Remove from rawBuffer
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
				static_cast<WireframeRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME])->ClearSpecific(component);
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
	m_RenderComponents.clear();
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME]->Clear();
	m_pScenePrimaryCamera = nullptr;
	static_cast<WireframeRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME])->Clear();
	m_BoundingBoxesToBePicked.clear();
}

void Renderer::submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->Submit(Upload_Default_Data);
}

void Renderer::submitMeshToCodt(Mesh* mesh)
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);

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
	}

	AssetLoader::Get()->m_LoadedModels.at(model->GetPath()).first = true;

	// TODO: Better structure instead of hardcoding here
	// Submit model to BLAS
	static_cast<BottomLevelRenderTask*>(m_DX12Tasks[E_DX12_TASK_TYPE::BLAS])->SubmitBLAS(model->m_pBLAS);
}

void Renderer::submitSlotInfoRawBufferToGPU(component::ModelComponent* mc)
{
	// Slotinfo
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(mc->m_SlotInfos.data());
	codt->Submit(&std::make_tuple(
		mc->m_SlotInfoByteAdressBuffer->GetUploadResource(),
		mc->m_SlotInfoByteAdressBuffer->GetDefaultResource(),
		data));
}

void Renderer::submitMaterialToGPU(component::ModelComponent* mc)
{
	// Submit Textures
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Material* mat = mc->GetMaterialAt(i);

		Texture* texture;
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::ALBEDO);
		submitTextureToCodt(texture);
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::ROUGHNESS);
		submitTextureToCodt(texture);
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::METALLIC);
		submitTextureToCodt(texture);
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::NORMAL);
		submitTextureToCodt(texture);
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::EMISSIVE);
		submitTextureToCodt(texture);
		texture = mat->GetTexture(E_TEXTURE2D_TYPE::OPACITY);
		submitTextureToCodt(texture);
	}
	
	// MaterialData
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(mc->m_MaterialDataRawBuffer.data());
	codt->Submit(&std::make_tuple(
		mc->m_MaterialByteAdressBuffer->GetUploadResource(),
		mc->m_MaterialByteAdressBuffer->GetDefaultResource(),
		data));
}

void Renderer::submitTextureToCodt(Texture* texture)
{
	if (AssetLoader::Get()->IsTextureLoadedOnGpu(texture) == true)
	{
		return;
	}

	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);
	codt->SubmitTexture(texture);

	AssetLoader::Get()->m_LoadedTextures.at(texture->GetPath()).first = true;
}


Entity* const Renderer::GetPickedEntity() const
{
	return m_pPickedEntity;
}

Scene* const Renderer::GetActiveScene() const
{
	return m_pCurrActiveScene;
}

void Renderer::setRenderTasksPrimaryCamera()
{
	m_RenderTasks[E_RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_LIGHT]->SetCamera(m_pScenePrimaryCamera);	// TEMP
	m_RenderTasks[E_RENDER_TASK_TYPE::OPACITY]->SetCamera(m_pScenePrimaryCamera);
	m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE]->SetCamera(m_pScenePrimaryCamera);

	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME]->SetCamera(m_pScenePrimaryCamera);
	}
}

void Renderer::createMainDSV()
{
	ID3D12Device5* m_pDevice5 = D3D12GraphicsManager::GetInstance()->m_pDevice5;
	DescriptorHeap* mainHeap = D3D12GraphicsManager::GetInstance()->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap  = D3D12GraphicsManager::GetInstance()->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap  = D3D12GraphicsManager::GetInstance()->GetDSVDescriptorHeap();
	ID3D12CommandQueue* pDirectQueue = D3D12GraphicsManager::GetInstance()->m_pGraphicsCommandQueue;

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_pMainDepthStencil = new DepthStencil(
		m_pDevice5,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		L"MainDSV",
		&dsvDesc,
		dsvHeap,
		mainHeap);
}

void Renderer::createFullScreenQuad()
{
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();
	ID3D12CommandQueue* pDirectQueue = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pGraphicsCommandQueue;

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
	vertices[3].uv = { 1.0f, 1.0f };

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
	m_pFullScreenQuad->Init(m_pDevice5, mainHeap);
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

		static_cast<OutliningRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE])->SetObjectToOutline(&std::make_pair(mc, tc));

		m_pPickedEntity = parentOfPickedObject;
	}
	else
	{
		// No object was picked, reset the outlingRenderTask
		static_cast<OutliningRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE])->Clear();
		m_pPickedEntity = nullptr;
	}
}

void Renderer::initRenderTasks()
{
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();
	ID3D12CommandQueue* pDirectQueue = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pGraphicsCommandQueue;

#pragma region DXRTasks
	DX12Task* blasTask = new BottomLevelRenderTask(m_pDevice5, F_THREAD_FLAGS::RENDER, L"BL_RenderTask_CommandList");
	DX12Task* tlasTask = new TopLevelRenderTask(m_pDevice5, F_THREAD_FLAGS::RENDER, L"TL_RenderTask_CommandList");

	TODO(Create all PSOs in corresponding classes like reflectionTask is done, instead of out here);
	// Everything is created inside here
	DXRTask* reflectionTask = new DXRReflectionTask(
		m_pDevice5,
		&m_ReflectionTexture,
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		F_THREAD_FLAGS::RENDER);

	reflectionTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	reflectionTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	reflectionTask->AddResource("rawBufferLights", Light::m_pLightsRawBuffer->GetDefaultResource());
	reflectionTask->AddResource("mainDSV", const_cast<Resource*>(m_pMainDepthStencil->GetDefaultResource()));	// To transition from depthWrite to depthRead
	
#pragma endregion DXRTasks

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
		L"DepthVertex.hlsl", L"DepthPixel.hlsl",
		&gpsdDepthPrePassVector,
		L"DepthPrePassPSO",
		F_THREAD_FLAGS::RENDER);

	DepthPrePassRenderTask->SetMainDepthStencil(m_pMainDepthStencil);

#pragma endregion DepthPrePass

#pragma region DeferredRenderingGeometry
	/* Depth Pre-Pass rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDeferredGeometryPass = {};
	gpsdDeferredGeometryPass.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// RenderTarget (TODO: Formats are way to big atm)
	gpsdDeferredGeometryPass.NumRenderTargets = 4;
	gpsdDeferredGeometryPass.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdDeferredGeometryPass.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdDeferredGeometryPass.RTVFormats[2] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdDeferredGeometryPass.RTVFormats[3] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdDeferredGeometryPass.SampleDesc.Count = 1;
	gpsdDeferredGeometryPass.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDeferredGeometryPass.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDeferredGeometryPass.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDeferredGeometryPass.RasterizerState.DepthBias = 0;
	gpsdDeferredGeometryPass.RasterizerState.DepthBiasClamp = 0.0f;
	gpsdDeferredGeometryPass.RasterizerState.SlopeScaledDepthBias = 0.0f;
	gpsdDeferredGeometryPass.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	// copy of defaultRTdesc
	D3D12_RENDER_TARGET_BLEND_DESC deferredGeometryRTdesc = {
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDeferredGeometryPass.BlendState.RenderTarget[i] = deferredGeometryRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC deferredGeometryDsd = {};
	deferredGeometryDsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	deferredGeometryDsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	deferredGeometryDsd.DepthEnable = true;

	// DepthStencil
	deferredGeometryDsd.StencilEnable = false;
	gpsdDeferredGeometryPass.DepthStencilState = deferredGeometryDsd;
	gpsdDeferredGeometryPass.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDeferredGeometryPassVector;
	gpsdDeferredGeometryPassVector.push_back(&gpsdDeferredGeometryPass);

	RenderTask* deferredGeometryRenderTask = new DeferredGeometryRenderTask(
		m_pDevice5,
		L"DeferredGeometryVertex.hlsl", L"DeferredGeometryPixel.hlsl",
		&gpsdDeferredGeometryPassVector,
		L"DeferredGeometryRenderTaskPSO",
		F_THREAD_FLAGS::RENDER);

	deferredGeometryRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	deferredGeometryRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	deferredGeometryRenderTask->AddRenderTargetView("gBufferAlbedo", m_GBufferAlbedo.first->GetRTV());
	deferredGeometryRenderTask->AddRenderTargetView("gBufferNormal", m_GBufferNormal.first->GetRTV());
	deferredGeometryRenderTask->AddRenderTargetView("gBufferMaterialProperties", m_GBufferMaterialProperties.first->GetRTV());
	deferredGeometryRenderTask->AddRenderTargetView("gBufferEmissive", m_GBufferEmissive.first->GetRTV());
#pragma endregion DeferredRenderingGeometry

#pragma region DeferredRenderingLight
	/* Deferred rendering without stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdDeferredLightRender = {};
	gpsdDeferredLightRender.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// RenderTarget
	gpsdDeferredLightRender.NumRenderTargets = 2;
	gpsdDeferredLightRender.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	gpsdDeferredLightRender.RTVFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
	// Depthstencil usage
	gpsdDeferredLightRender.SampleDesc.Count = 1;
	gpsdDeferredLightRender.SampleMask = UINT_MAX;
	// Rasterizer behaviour
	gpsdDeferredLightRender.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsdDeferredLightRender.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	gpsdDeferredLightRender.RasterizerState.FrontCounterClockwise = false;

	// Specify Blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc = {
		false, false,
		D3D12_BLEND_ZERO, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL };
	for (unsigned int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsdDeferredLightRender.BlendState.RenderTarget[i] = defaultRTdesc;

	// Depth descriptor
	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = false;
	dsd.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	dsd.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencil
	dsd.StencilEnable = false;
	gpsdDeferredLightRender.DepthStencilState = dsd;
	gpsdDeferredLightRender.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	/* Forward rendering with stencil testing */
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdForwardRenderStencilTest = gpsdDeferredLightRender;

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

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDeferredLightRenderVector;
	gpsdDeferredLightRenderVector.push_back(&gpsdDeferredLightRender);
	gpsdDeferredLightRenderVector.push_back(&gpsdForwardRenderStencilTest);

	RenderTask* deferredLightRenderTask = new DeferredLightRenderTask(
		m_pDevice5,
		L"DeferredLightVertex.hlsl", L"DeferredLightPixel.hlsl",
		&gpsdDeferredLightRenderVector,
		L"DeferredLightRenderingPSO",
		F_THREAD_FLAGS::RENDER);

	deferredLightRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	deferredLightRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	deferredLightRenderTask->AddResource("rawBufferLights", Light::m_pLightsRawBuffer->GetDefaultResource());
	deferredLightRenderTask->SetMainDepthStencil(m_pMainDepthStencil);

	deferredLightRenderTask->AddRenderTargetView("brightTarget", std::get<1>(*m_pBloomResources->GetBrightTuple()));
	deferredLightRenderTask->AddRenderTargetView("finalColorBuffer", m_FinalColorBuffer.first->GetRTV());

	static_cast<DeferredLightRenderTask*>(deferredLightRenderTask)->SetFullScreenQuad(m_pFullScreenQuad);

#pragma endregion DeferredRenderingLight

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
		L"DownSampleVertex.hlsl", L"DownSamplePixel.hlsl",
		&gpsdDownSampleTextureVector,
		L"DownSampleTexturePSO",
		std::get<2>(*m_pBloomResources->GetBrightTuple()),		// Read from this in actual resolution
		m_pBloomResources->GetPingPongResource(0)->GetRTV(),	// Write to this in 1280x720
		F_THREAD_FLAGS::RENDER);
	
	static_cast<DownSampleRenderTask*>(downSampleTask)->SetFullScreenQuad(m_pFullScreenQuad);
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
		L"OutlinedVertex.hlsl", L"OutlinedPixel.hlsl",
		&gpsdOutliningVector,
		L"outliningScaledPSO",
		mainHeap,
		F_THREAD_FLAGS::RENDER);
	
	outliningRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	outliningRenderTask->AddRenderTargetView("finalColorBuffer", m_FinalColorBuffer.first->GetRTV());

#pragma endregion ModelOutlining

#pragma region Blend
	// ------------------------ BLEND ---------------------------- FRONTCULL

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendFrontCull = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsdBlendBackCull = {};
	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdBlendVector;

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

	// ------------------------ BLEND ---------------------------- BACKCULL

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

	gpsdBlendBackCull.DepthStencilState = dsdBlend;
	gpsdBlendBackCull.DSVFormat = m_pMainDepthStencil->GetDSV()->GetDXGIFormat();

	// Push back to vector
	gpsdBlendVector.push_back(&gpsdBlendFrontCull);
	gpsdBlendVector.push_back(&gpsdBlendBackCull);
	
	/*---------------------------------- TRANSPARENT_TEXTURE_RENDERTASK -------------------------------------*/
	RenderTask* transparentRenderTask = new TransparentRenderTask(m_pDevice5,
		L"TransparentTextureVertex.hlsl",
		L"TransparentTexturePixel.hlsl",
		&gpsdBlendVector,
		L"BlendPSOTexture",
		F_THREAD_FLAGS::RENDER);

	transparentRenderTask->AddResource("cbPerFrame", m_pCbPerFrame->GetDefaultResource());
	transparentRenderTask->AddResource("cbPerScene", m_pCbPerScene->GetDefaultResource());
	transparentRenderTask->AddResource("rawBufferLights", Light::m_pLightsRawBuffer->GetDefaultResource());
	transparentRenderTask->SetMainDepthStencil(m_pMainDepthStencil);
	transparentRenderTask->AddRenderTargetView("finalColorBuffer", m_FinalColorBuffer.first->GetRTV());

#pragma endregion Blend

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
		L"WhiteVertex.hlsl", L"WhitePixel.hlsl",
		&gpsdWireFrameVector,
		L"WireFramePSO",
		F_THREAD_FLAGS::RENDER);

	wireFrameRenderTask->AddRenderTargetView("finalColorBuffer", m_FinalColorBuffer.first->GetRTV());
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
		L"MergeVertex.hlsl", L"MergePixel.hlsl",
		&gpsdMergePassVector,
		L"MergePassPSO",
		F_THREAD_FLAGS::RENDER);

	static_cast<MergeRenderTask*>(mergeTask)->SetFullScreenQuad(m_pFullScreenQuad);
	static_cast<MergeRenderTask*>(mergeTask)->AddSRVToMerge(m_pBloomResources->GetPingPongResource(0)->GetSRV());
	static_cast<MergeRenderTask*>(mergeTask)->AddSRVToMerge(m_FinalColorBuffer.second);
	static_cast<MergeRenderTask*>(mergeTask)->AddSRVToMerge(m_ReflectionTexture.srv);
	static_cast<MergeRenderTask*>(mergeTask)->CreateSlotInfo();
#pragma endregion MergePass

#pragma region IMGUIRENDERTASK
	RenderTask* imGuiRenderTask = new ImGuiRenderTask(
		m_pDevice5,
		L"", L"",
		nullptr,
		L"",
		F_THREAD_FLAGS::RENDER);
#pragma endregion IMGUIRENDERTASK

#pragma region ComputeAndCopyTasks
	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurHorizontal.hlsl", L"blurHorizontalPSO"));
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurVertical.hlsl", L"blurVerticalPSO"));
	ComputeTask* blurComputeTask = new BlurComputeTask(
		m_pDevice5,
		csNamePSOName,
		E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE,
		std::get<2>(*m_pBloomResources->GetBrightTuple()),
		m_pBloomResources->GetPingPongResource(0),
		m_pBloomResources->GetPingPongResource(1),
		m_pBloomResources->GetBlurWidth(), m_pBloomResources->GetBlurHeight(),
		F_THREAD_FLAGS::RENDER);

	// CopyTasks
	CopyTask* copyPerFrameTask			= new CopyPerFrameTask(m_pDevice5, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, F_THREAD_FLAGS::RENDER, L"copyPerFrameCL");
	CopyTask* copyPerFrameMatricesTask  = new CopyPerFrameMatricesTask(m_pDevice5, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, F_THREAD_FLAGS::RENDER, L"copyPerFrameMatricesCL");
	CopyTask* copyOnDemandTask			= new CopyOnDemandTask(m_pDevice5, E_COMMAND_INTERFACE_TYPE::DIRECT_TYPE, F_THREAD_FLAGS::RENDER, L"copyOnDemandCL");

#pragma endregion ComputeAndCopyTasks

	// Add the tasks to desired vectors so they can be used in m_pRenderer
	/* -------------------------------------------------------------- */


	/* ------------------------- CopyQueue Tasks ------------------------ */

	m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME] = copyPerFrameTask;
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME_MATRICES] = copyPerFrameMatricesTask;
	m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND] = copyOnDemandTask;

	/* ------------------------- ComputeQueue Tasks ------------------------ */
	
	m_ComputeTasks[E_COMPUTE_TASK_TYPE::BLUR] = blurComputeTask;

	/* ------------------------- DirectQueue Tasks ---------------------- */
	m_RenderTasks[E_RENDER_TASK_TYPE::DEPTH_PRE_PASS] = DepthPrePassRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY] = deferredGeometryRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_LIGHT] = deferredLightRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::OPACITY] = transparentRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::WIREFRAME] = wireFrameRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::OUTLINE] = outliningRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::MERGE] = mergeTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::IMGUI] = imGuiRenderTask;
	m_RenderTasks[E_RENDER_TASK_TYPE::DOWNSAMPLE] = downSampleTask;

	/* ------------------------- DX12 Tasks ------------------------ */
	m_DX12Tasks[E_DX12_TASK_TYPE::BLAS] = blasTask;
	m_DX12Tasks[E_DX12_TASK_TYPE::TLAS] = tlasTask;

	/* ------------------------- DXR Tasks ------------------------ */
	m_DXRTasks[E_DXR_TASK_TYPE::REFLECTIONS] = reflectionTask;

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
		m_DirectCommandLists[i].push_back(copyPerFrameMatricesTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		// Update the BLAS:es if any (synced-version)
		m_DirectCommandLists[i].push_back(blasTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		// Update the TLAS every frame
		m_DirectCommandLists[i].push_back(tlasTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(DepthPrePassRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(deferredGeometryRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(reflectionTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(deferredLightRenderTask->GetCommandInterface()->GetCommandList(i));
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(downSampleTask->GetCommandInterface()->GetCommandList(i));
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

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_DirectCommandLists[i].push_back(transparentRenderTask->GetCommandInterface()->GetCommandList(i));
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

	// -------------------------------------- GUI -------------------------------------------------
	// Debug/ImGui
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_ImGuiCommandLists[i].push_back(imGuiRenderTask->GetCommandInterface()->GetCommandList(i));
	}
}

void Renderer::createRawBufferForLights()
{
	// TODO ABSTRACTION
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();
	ID3D12CommandQueue* pDirectQueue = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pGraphicsCommandQueue;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = 1; // Wierd to specify this?? But crashes otherwise.
	//srvDesc.Buffer.StructureByteStride = sizeof(unsigned int);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	unsigned int rawBufferSize   = sizeof(LightHeader)		+
				MAX_DIR_LIGHTS	 * sizeof(DirectionalLight) +
				MAX_POINT_LIGHTS * sizeof(PointLight)		+
				MAX_SPOT_LIGHTS  * sizeof(SpotLight);

	Light::m_pLightsRawBuffer = new ShaderResource(m_pDevice5, rawBufferSize, L"rawBufferLights", &srvDesc, mainHeap);

	// Allocate memory on CPU
	Light::m_pRawData = (unsigned char*)malloc(rawBufferSize);
	memset(Light::m_pRawData, 0, rawBufferSize);
}

void Renderer::setRenderTasksRenderComponents()
{
	m_RenderTasks[E_RENDER_TASK_TYPE::DEPTH_PRE_PASS]->SetRenderComponents(&m_RenderComponents[F_DRAW_FLAGS::NO_DEPTH]);
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY]->SetRenderComponents(&m_RenderComponents[F_DRAW_FLAGS::DRAW_OPAQUE]);
	m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_LIGHT]->SetRenderComponents(&m_RenderComponents[F_DRAW_FLAGS::DRAW_OPAQUE]);	// TEMP
	m_RenderTasks[E_RENDER_TASK_TYPE::OPACITY]->SetRenderComponents(&m_RenderComponents[F_DRAW_FLAGS::DRAW_TRANSPARENT]);
}

void Renderer::prepareScene(Scene* activeScene)
{
	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();
	ID3D12CommandQueue* pDirectQueue = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pGraphicsCommandQueue;

	m_pCurrActiveScene = activeScene;
	submitUploadPerFrameData();
	submitUploadPerSceneData();

	// -------------------- DEBUG STUFF --------------------
	// Test to change m_pCamera to the shadow casting m_lights cameras
	//auto& tuple = m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].at(0);
	//BaseCamera* tempCam = std::get<0>(tuple)->GetCamera();
	//m_pScenePrimaryCamera = tempCam;

	if (m_pScenePrimaryCamera == nullptr)
	{
		BL_LOG_CRITICAL("No primary camera was set in scenes\n");

		// Todo: Set default m_pCamera
	}

	m_pMousePicker->SetPrimaryCamera(m_pScenePrimaryCamera);

	setRenderTasksRenderComponents();
	setRenderTasksPrimaryCamera();

	// DXR, create buffers and create for the first time
	TopLevelAccelerationStructure* pTLAS = static_cast<TopLevelRenderTask*>(m_DX12Tasks[E_DX12_TASK_TYPE::TLAS])->GetTLAS();

	unsigned int i = 0;
	for (RenderComponent rc : m_RayTracedRenderComponents)
	{
		pTLAS->AddInstance(rc.mc->GetModel()->m_pBLAS, *rc.tc->GetTransform()->GetWorldMatrix(), i);	// TODO: HitgroupID
		i++;
	}
	pTLAS->GenerateBuffers(m_pDevice5, mainHeap);
	pTLAS->SetupAccelerationStructureForBuilding(m_pDevice5, false);

	static_cast<DXRReflectionTask*>(m_DXRTasks[E_DXR_TASK_TYPE::REFLECTIONS])->CreateShaderBindingTable(m_pDevice5, m_RayTracedRenderComponents);
	// Currently unused
	//static_cast<DeferredLightRenderTask*>(m_RenderTasks[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY])->SetSceneBVHSRV(pTLAS->GetSRV());

	// PerSceneData 
	m_pCbPerSceneData->rayTracingBVH			 = pTLAS->GetSRV()->GetDescriptorHeapIndex();
	m_pCbPerSceneData->gBufferAlbedo			 = m_GBufferAlbedo.second->GetDescriptorHeapIndex();
	m_pCbPerSceneData->gBufferNormal			 = m_GBufferNormal.second->GetDescriptorHeapIndex();
	m_pCbPerSceneData->gBufferMaterialProperties = m_GBufferMaterialProperties.second->GetDescriptorHeapIndex();
	m_pCbPerSceneData->gBufferEmissive			 = m_GBufferEmissive.second->GetDescriptorHeapIndex();
	m_pCbPerSceneData->depth					 = m_pMainDepthStencil->GetSRV()->GetDescriptorHeapIndex();
	m_pCbPerSceneData->reflectionUAV			 = m_ReflectionTexture.uav->GetDescriptorHeapIndex();
	m_pCbPerSceneData->reflectionSRV			 = m_ReflectionTexture.srv->GetDescriptorHeapIndex();
}

void Renderer::submitUploadPerSceneData()
{
	// Submit CB_PER_SCENE to be uploaded to VRAM
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_ON_DEMAND]);
	const void* data = static_cast<const void*>(m_pCbPerSceneData);
	codt->Submit(&std::make_tuple(m_pCbPerScene->GetUploadResource(), m_pCbPerScene->GetDefaultResource(), data));
}

void Renderer::submitUploadPerFrameData()
{
	// Submit dynamic-light-data to be uploaded to VRAM
	CopyPerFrameTask* cpft = static_cast<CopyPerFrameTask*>(m_CopyTasks[E_COPY_TASK_TYPE::COPY_PER_FRAME]);

	// CB_PER_FRAME_STRUCT
	if (cpft != nullptr)
	{
		const void* data = static_cast<void*>(m_pCbPerFrameData);
		cpft->Submit(&std::tuple(m_pCbPerFrame->GetUploadResource(), m_pCbPerFrame->GetDefaultResource(), data));
	}

	// All lights (data and header with information)
	// Sending entire buffer (4kb as of writing this code), every frame for simplicity. Even if some lights might be static.
	cpft->Submit(&std::make_tuple(
		Light::m_pLightsRawBuffer->GetUploadResource(),
		Light::m_pLightsRawBuffer->GetDefaultResource(),
		const_cast<const void*>(static_cast<void*>(Light::m_pRawData))));	// Yes, its great!

}

//void Renderer::toggleFullscreen(WindowChange* event)
//{
//	ID3D12Device5* m_pDevice5 = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pDevice5;
//	DescriptorHeap* mainHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetMainDescriptorHeap();
//	DescriptorHeap* rtvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetRTVDescriptorHeap();
//	DescriptorHeap* dsvHeap = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->GetDSVDescriptorHeap();
//	ID3D12CommandQueue* pDirectQueue = static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->m_pGraphicsCommandQueue;
//
//	m_FenceFrameValue++;
//	pDirectQueue->Signal(m_pFenceFrame, m_FenceFrameValue);
//
//	// Wait for all frames
//	waitForFrame(0);
//
//	// Wait for the threads which records the commandlists to complete
//	m_pThreadPool->WaitForThreads(F_THREAD_FLAGS::RENDER);
//
//	for (auto task : m_RenderTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->Reset(i);
//		}
//	}
//	for (auto task : m_CopyTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->Reset(i);
//		}
//	}
//	for (auto task : m_ComputeTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->Reset(i);
//		}
//	}
//
//	m_pSwapChain->ToggleWindowMode(m_pDevice5,
//		m_pWindow->GetHwnd(),
//		pDirectQueue,
//		rtvHeap,
//		mainHeap);
//
//	// Change the member variables of the window class to match the swapchain
//	UINT width = 0, height = 0;
//	if (m_pSwapChain->IsFullscreen())
//	{
//		m_pSwapChain->GetDX12SwapChain()->GetSourceSize(&width, &height);
//	}
//	else
//	{
//		// Earlier it read from options. now just set to 800/600
//		width = 800;
//		height = 600;
//	}
//
//	Window* window = const_cast<Window*>(m_pWindow);
//	window->SetScreenWidth(width);
//	window->SetScreenHeight(height);
//
//	for (auto task : m_RenderTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->GetCommandList(i)->Close();
//		}
//	}
//	for (auto task : m_CopyTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->GetCommandList(i)->Close();
//		}
//	}
//	for (auto task : m_ComputeTasks)
//	{
//		for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
//		{
//			task->GetCommandInterface()->GetCommandList(i)->Close();
//		}
//	}
//}
