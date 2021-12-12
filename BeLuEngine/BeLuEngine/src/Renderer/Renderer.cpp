#include "stdafx.h"
#include "Renderer.h"

// Misc
#include "../Misc/AssetLoader.h"
#include "../Misc/MultiThreading/ThreadPool.h"
#include "../Misc/MultiThreading/Thread.h"
#include "../Misc/Window.h"
#include "DXILShaderCompiler.h"

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
#include "DescriptorHeap.h"
#include "Geometry/Transform.h"
#include "Camera/BaseCamera.h"
#include "Geometry/Model.h"
#include "Geometry/Mesh.h"
#include "Geometry/Material.h"

// Techniques
#include "Techniques/Bloom.h"
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
#include "DX12Tasks/CopyPerFrameMatricesTask.h"
#include "DX12Tasks/CopyOnDemandTask.h"
#include "DX12Tasks/BlurComputeTask.h"
#include "DX12Tasks/BottomLevelRenderTask.h"
#include "DX12Tasks/TopLevelRenderTask.h"
#include "DX12Tasks/DXRReflectionTask.h"
#include "DXR/BottomLevelAccelerationStructure.h"
#include "DXR/TopLevelAccelerationStructure.h"

// Event
#include "../Events/EventBus.h"

// ABSTRACTION TEMP
#include "API/D3D12/D3D12GraphicsManager.h"
#include "API/D3D12/D3D12GraphicsBuffer.h"
#include "API/D3D12/D3D12GraphicsTexture.h"

#include "API/IGraphicsBuffer.h"

Renderer::Renderer()
{
	//EventBus::GetInstance().Subscribe(this, &Renderer::toggleFullscreen);
	m_GraphicsPasses.resize(E_GRAPHICS_PASS_TYPE::NR_OF_GRAPHICS_PASSES);
	
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

	// Delete Textures
	BL_SAFE_DELETE(m_pFullScreenQuad);
	BL_SAFE_DELETE(m_FinalColorBuffer);
	BL_SAFE_DELETE(m_GBufferAlbedo);
	BL_SAFE_DELETE(m_GBufferNormal);
	BL_SAFE_DELETE(m_GBufferMaterialProperties);
	BL_SAFE_DELETE(m_GBufferEmissive);
	BL_SAFE_DELETE(m_ReflectionTexture);

	// Delete Depthbuffer
	BL_SAFE_DELETE(m_pMainDepthStencil);

	BL_SAFE_DELETE(m_pBloomWrapperTemp);

	for (GraphicsPass* graphicsPass : m_GraphicsPasses)
		BL_SAFE_DELETE(graphicsPass);

	BL_SAFE_DELETE(m_pMousePicker);

	BL_SAFE_DELETE(m_pCbPerScene);
	BL_SAFE_DELETE(m_pCbPerSceneData);
	BL_SAFE_DELETE(m_pCbPerFrame);
	BL_SAFE_DELETE(m_pCbPerFrameData);

	BL_SAFE_DELETE(Light::m_pLightsRawBuffer);
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
	m_FinalColorBuffer = IGraphicsTexture::Create();
	m_FinalColorBuffer->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"FinalColorbuffer",
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// GBufferAlbedo
	m_GBufferAlbedo = IGraphicsTexture::Create();
	m_GBufferAlbedo->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferAlbedo",
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Normal
	m_GBufferNormal = IGraphicsTexture::Create();
	m_GBufferNormal->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferNormal",
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Material Properties (Roughness, Metallic, glow..
	m_GBufferMaterialProperties = IGraphicsTexture::Create();
	m_GBufferMaterialProperties->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferMaterials",
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Emissive Color
	m_GBufferEmissive = IGraphicsTexture::Create();
	m_GBufferEmissive->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferEmissive",
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Resource
	m_ReflectionTexture = IGraphicsTexture::Create();
	m_ReflectionTexture->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::UnorderedAccess,
		L"ReflectionTexture",
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// DepthBuffer
	m_pMainDepthStencil = IGraphicsTexture::Create();
	m_pMainDepthStencil->CreateTexture2D(
		m_CurrentRenderingWidth, m_CurrentRenderingHeight,
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::DepthStencil,
		L"MainDepthStencilBuffer",
		D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// Bloom
	m_pBloomWrapperTemp = new Bloom();

	
#pragma endregion RenderTargets

	// Picking
	m_pMousePicker = new MousePicker();
	
	DXILShaderCompiler::Get()->Init();

	// FullScreenQuad
	createFullScreenQuad();

	// Init Assetloader (Note: This should be free to use before this line, before it wasn't)
	AssetLoader* al = AssetLoader::Get();

	// Init BoundingBoxPool
	BoundingBoxPool::Get(m_pDevice5, mainHeap);

	// Allocate memory for cbPerScene
	m_pCbPerScene = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(CB_PER_SCENE_STRUCT), 1, DXGI_FORMAT_UNKNOWN, L"CB_PER_SCENE");
	m_pCbPerSceneData = new CB_PER_SCENE_STRUCT();

	// Allocate memory for cbPerFrame
	m_pCbPerFrame = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(CB_PER_FRAME_STRUCT), 1, DXGI_FORMAT_UNKNOWN, L"CB_PER_FRAME");
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

	initGraphicsPasses();

	// Submit fullscreenquad
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	codt->SubmitBuffer(m_pFullScreenQuad->GetVertexBuffer(), m_pFullScreenQuad->GetVertices()->data());
	codt->SubmitBuffer(m_pFullScreenQuad->GetIndexBuffer(), m_pFullScreenQuad->GetIndices()->data());
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

	submitUploadPerFrameData();

	// Submit Transforms
	

	// Picking
	updateMousePicker();

	

	// ImGui
#ifdef DEBUG
	ImGuiHandler::GetInstance().NewFrame();
#endif

	// DXR
	TopLevelAccelerationStructure* pTLAS = static_cast<TopLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS])->GetTLAS();

	pTLAS->Reset();

	unsigned int i = 0;
	for (RenderComponent rc : m_RayTracedRenderComponents)
	{
		pTLAS->AddInstance(rc.mc->GetModel()->m_pBLAS, *rc.tc->GetTransform()->GetWorldMatrix(), i);
		i++;
	}

	pTLAS->SetupAccelerationStructureForBuilding(true);
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

	GraphicsPass* graphicsPass = nullptr;
	/* --------------------- Record command lists --------------------- */

	// Copy on demand
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND];
	m_pThreadPool->AddTask(graphicsPass);
	
	// Copy per frame (matrices, world and wvp)
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES];
	m_pThreadPool->AddTask(graphicsPass);

	// Update BLASes if any...
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS];
	m_pThreadPool->AddTask(graphicsPass);

	// Depth pre-pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS];
	m_pThreadPool->AddTask(graphicsPass);

	// Update TLAS
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS];
	m_pThreadPool->AddTask(graphicsPass);

	// Geometry pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY];
	graphicsPass->Execute();

	// Light pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_LIGHT];
	graphicsPass->Execute();

	// DXR Reflections
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS];
	m_pThreadPool->AddTask(graphicsPass);

	// DownSample the texture used for bloom
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DOWNSAMPLE];
	m_pThreadPool->AddTask(graphicsPass);

	// Blend
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY];
	m_pThreadPool->AddTask(graphicsPass);

	// Blurring for bloom
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLOOM];
	m_pThreadPool->AddTask(graphicsPass);

	// Outlining, if an object is picked
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE];
	m_pThreadPool->AddTask(graphicsPass);

	// Merge 
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::MERGE];
	m_pThreadPool->AddTask(graphicsPass);
	
	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME];
		m_pThreadPool->AddTask(graphicsPass);
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */

	// Wait for the threads which records the commandlists to complete
	m_pThreadPool->WaitForThreads(F_THREAD_FLAGS::GRAPHICS);


	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_MainGraphicsContexts, m_MainGraphicsContexts.size());
	/* --------------------------------------------------------------- */

	// ImGui
#ifdef DEBUG
	// Have to update ImGui here to get all information that happens inside rendering
	ImGuiHandler::GetInstance().UpdateFrame();

	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::IMGUI];
	graphicsPass->Execute();

	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_ImGuiGraphicsContext, m_ImGuiGraphicsContext.size());
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

	GraphicsPass* graphicsPass = nullptr;

	/* --------------------- Record command lists --------------------- */
	// Copy on demand
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND];
	graphicsPass->Execute();
	
	// Copy per frame (matrices, world and wvp)
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES];
	graphicsPass->Execute();

	// Update BLASes if any...
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS];
	graphicsPass->Execute();

	// Depth pre-pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS];
	graphicsPass->Execute();

	// Update TLAS
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS];
	graphicsPass->Execute();

	// Geometry pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY];
	graphicsPass->Execute();

	// Light pass
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_LIGHT];
	graphicsPass->Execute();

	// DXR Reflections
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS];
	graphicsPass->Execute();

	// DownSample the texture used for bloom
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DOWNSAMPLE];
	graphicsPass->Execute();

	// Blend
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY];
	graphicsPass->Execute();

	// Blurring for bloom
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLOOM];
	graphicsPass->Execute();

	// Outlining, if an object is picked
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE];
	graphicsPass->Execute();

	// Merge 
	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::MERGE];
	graphicsPass->Execute();

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME];
		graphicsPass->Execute();
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_MainGraphicsContexts, m_MainGraphicsContexts.size());

	/* --------------------------------------------------------------- */

	/*------------------- Post draw stuff -------------------*/

	// ImGui
#ifdef DEBUG
	// Have to update ImGui here to get all information that happens inside rendering
	ImGuiHandler::GetInstance().UpdateFrame();

	graphicsPass = m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::IMGUI];
	graphicsPass->Execute();

	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->Execute(m_ImGuiGraphicsContext, m_ImGuiGraphicsContext.size());
#endif

	/*------------------- Present -------------------*/
	static_cast<D3D12GraphicsManager*>(D3D12GraphicsManager::GetInstance())->SyncAndPresent();

	// Check to end ImGui if its active
	ImGuiHandler::GetInstance().EndFrame();

	graphicsManager->End();
}

void Renderer::InitModelComponent(component::ModelComponent* mc)
{
	component::TransformComponent* tc = mc->GetParent()->GetComponent<component::TransformComponent>();

	// Submit to codt
	submitModelToGPU(mc->m_pModel);

	mc->UpdateMaterialRawBufferFromMaterial();
	submitMaterialToGPU(mc);

	// Only add the m_Entities that actually should be drawn
	if (tc != nullptr)
	{
		Transform* t = tc->GetTransform();
		TODO("Possible problem");
		t->m_pConstantBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(DirectX::XMMATRIX), 2, DXGI_FORMAT_UNKNOWN, L"Transform");

		t->UpdateWorldMatrix();
		static_cast<CopyPerFrameMatricesTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES])->SubmitTransform(t);

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

		CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
		codt->SubmitBuffer(mc->GetSlotInfoByteAdressBufferDXR(), mc->m_SlotInfos.data());
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
				CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
				codt->SubmitBuffer(mesh->GetVertexBuffer(), mesh->GetVertices()->data());
				codt->SubmitBuffer(mesh->GetIndexBuffer(), mesh->GetIndices()->data());
			}

			component->AddMesh(mesh);
		}
		static_cast<WireframeRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME])->AddObjectToDraw(component);
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
	TODO("Possible memory leak");
	//m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES]->ClearSpecific(tc->GetTransform()->m_pCB->GetUploadResource());

	// Delete constantBuffer for matrices
	if (tc->GetTransform()->m_pConstantBuffer != nullptr)
	{
		delete tc->GetTransform()->m_pConstantBuffer;
		tc->GetTransform()->m_pConstantBuffer = nullptr;
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
				static_cast<WireframeRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME])->ClearSpecific(component);
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
	m_pScenePrimaryCamera = nullptr;
	static_cast<WireframeRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME])->Clear();
	m_BoundingBoxesToBePicked.clear();
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
		CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
		codt->SubmitBuffer(mesh->GetVertexBuffer(), mesh->GetVertices()->data());
		codt->SubmitBuffer(mesh->GetIndexBuffer(), mesh->GetIndices()->data());
	}

	AssetLoader::Get()->m_LoadedModels.at(model->GetPath()).first = true;

	// TODO: Better structure instead of hardcoding here
	// Submit model to BLAS
	static_cast<BottomLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS])->SubmitBLAS(model->m_pBLAS);
}

void Renderer::submitMaterialToGPU(component::ModelComponent* mc)
{
	auto SubmitTextureLambda = [=](IGraphicsTexture* graphicsTexture)
	{
		if (AssetLoader::Get()->IsTextureLoadedOnGpu(graphicsTexture) == true)
		{
			return;
		}

		CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
		codt->SubmitTexture(graphicsTexture);

		AssetLoader::Get()->m_LoadedTextures.at(graphicsTexture->GetPath()).first = true;
	};

	// Submit Textures
	for (unsigned int i = 0; i < mc->GetNrOfMeshes(); i++)
	{
		Material* mat = mc->GetMaterialAt(i);

		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::ALBEDO));
		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::ROUGHNESS));
		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::METALLIC));
		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::NORMAL));
		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::EMISSIVE));
		SubmitTextureLambda(mat->GetTexture(E_TEXTURE2D_TYPE::OPACITY));
	}
	
	// MaterialData
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	codt->SubmitBuffer(mc->GetMaterialByteAdressBuffer(), mc->m_MaterialDataRawBuffer.data());
}

Entity* const Renderer::GetPickedEntity() const
{
	return m_pPickedEntity;
}

Scene* const Renderer::GetActiveScene() const
{
	return m_pCurrActiveScene;
}

void Renderer::setGraphicsPassesPrimaryCamera()
{
	static_cast<DepthRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS])->SetCamera(m_pScenePrimaryCamera);
	static_cast<DeferredGeometryRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY])->SetCamera(m_pScenePrimaryCamera);
	static_cast<TransparentRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY])->SetCamera(m_pScenePrimaryCamera);
	static_cast<OutliningRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE])->SetCamera(m_pScenePrimaryCamera);
	static_cast<CopyPerFrameMatricesTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES])->SetCamera(m_pScenePrimaryCamera);
	static_cast<WireframeRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME])->SetCamera(m_pScenePrimaryCamera);
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
	m_pFullScreenQuad->Init();
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

		static_cast<OutliningRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE])->SetObjectToOutline(&std::make_pair(mc, tc));

		m_pPickedEntity = parentOfPickedObject;
	}
	else
	{
		// No object was picked, reset the outlingRenderTask
		static_cast<OutliningRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE])->Clear();
		m_pPickedEntity = nullptr;
	}
}

void Renderer::initGraphicsPasses()
{
	TODO(Create all PSOs in corresponding classes like reflectionTask is done, instead of out here);

#pragma region DXR
	GraphicsPass* blasTask = new BottomLevelRenderTask();
	GraphicsPass* tlasTask = new TopLevelRenderTask();

	// Everything is created inside here
	GraphicsPass* reflectionPassDXR = new DXRReflectionTask(m_ReflectionTexture, m_CurrentRenderingWidth, m_CurrentRenderingHeight);

	reflectionPassDXR->AddGraphicsBuffer("cbPerFrame", m_pCbPerFrame);
	reflectionPassDXR->AddGraphicsBuffer("cbPerScene", m_pCbPerScene);
	reflectionPassDXR->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);
	reflectionPassDXR->AddGraphicsTexture("mainDSV", m_pMainDepthStencil);	// To transition from depthWrite to depthRead

#pragma endregion DXRTasks

#pragma region DepthPrePass

	GraphicsPass* depthPrePass = new DepthRenderTask();

	depthPrePass->AddGraphicsTexture("mainDepthStencilBuffer", m_pMainDepthStencil);

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
	gpsdDeferredGeometryPass.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdDeferredGeometryPassVector;
	gpsdDeferredGeometryPassVector.push_back(&gpsdDeferredGeometryPass);

	GraphicsPass* deferredGeometryPass = new DeferredGeometryRenderTask();

	deferredGeometryPass->AddGraphicsBuffer("cbPerScene", m_pCbPerScene);
	deferredGeometryPass->AddGraphicsTexture("gBufferAlbedo", m_GBufferAlbedo);
	deferredGeometryPass->AddGraphicsTexture("gBufferNormal", m_GBufferNormal);
	deferredGeometryPass->AddGraphicsTexture("gBufferMaterialProperties", m_GBufferMaterialProperties);
	deferredGeometryPass->AddGraphicsTexture("gBufferEmissive", m_GBufferEmissive);
	deferredGeometryPass->AddGraphicsTexture("mainDepthStencilBuffer", m_pMainDepthStencil);
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
	gpsdDeferredLightRender.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

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

	GraphicsPass* deferredLightPass = new DeferredLightRenderTask(m_pFullScreenQuad);

	deferredLightPass->AddGraphicsBuffer("cbPerFrame", m_pCbPerFrame);
	deferredLightPass->AddGraphicsBuffer("cbPerScene", m_pCbPerScene);
	deferredLightPass->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);
	deferredLightPass->AddGraphicsTexture("mainDepthStencilBuffer", m_pMainDepthStencil);

	deferredLightPass->AddGraphicsTexture("brightTarget", m_pBloomWrapperTemp->GetBrightTexture());
	deferredLightPass->AddGraphicsTexture("finalColorBuffer", m_FinalColorBuffer);
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

	GraphicsPass* downSamplePass = new DownSampleRenderTask(
		m_pBloomWrapperTemp->GetBrightTexture(),		// Read from this in actual resolution
		m_pBloomWrapperTemp->GetPingPongTexture(0),		// Write to this in 1280x720
		m_pFullScreenQuad);
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
	gpsdModelOutlining.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*> gpsdOutliningVector;
	gpsdOutliningVector.push_back(&gpsdModelOutlining);

	GraphicsPass* outliningPass = new OutliningRenderTask();

	outliningPass->AddGraphicsTexture("mainDepthStencilBuffer", m_pMainDepthStencil);
	outliningPass->AddGraphicsTexture("finalColorBuffer", m_FinalColorBuffer);

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
	gpsdBlendFrontCull.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

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
	gpsdBlendBackCull.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Push back to vector
	gpsdBlendVector.push_back(&gpsdBlendFrontCull);
	gpsdBlendVector.push_back(&gpsdBlendBackCull);

	/*---------------------------------- TRANSPARENT_TEXTURE_RENDERTASK -------------------------------------*/
	GraphicsPass* transparentPass = new TransparentRenderTask();

	transparentPass->AddGraphicsBuffer("cbPerFrame", m_pCbPerFrame);
	transparentPass->AddGraphicsBuffer("cbPerScene", m_pCbPerScene);
	transparentPass->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);
	transparentPass->AddGraphicsTexture("mainDepthStencilBuffer", m_pMainDepthStencil);
	transparentPass->AddGraphicsTexture("finalColorBuffer", m_FinalColorBuffer);

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

	GraphicsPass* wireFramePass = new WireframeRenderTask();

	wireFramePass->AddGraphicsTexture("finalColorBuffer", m_FinalColorBuffer);
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

	GraphicsPass* mergePass = new MergeRenderTask(m_pFullScreenQuad);

	mergePass->AddGraphicsTexture("bloomPingPong0", m_pBloomWrapperTemp->GetPingPongTexture(0));
	mergePass->AddGraphicsTexture("finalColorBuffer", m_FinalColorBuffer);
	mergePass->AddGraphicsTexture("reflectionTexture", m_ReflectionTexture);
#pragma endregion MergePass

#pragma region IMGUIRENDERTASK
	GraphicsPass* imGuiPass = new ImGuiRenderTask();
#pragma endregion IMGUIRENDERTASK

#pragma region ComputeAndCopyTasks
	// ComputeTasks
	std::vector<std::pair<std::wstring, std::wstring>> csNamePSOName;
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurHorizontal.hlsl", L"blurHorizontalPSO"));
	csNamePSOName.push_back(std::make_pair(L"ComputeBlurVertical.hlsl", L"blurVerticalPSO"));
	GraphicsPass* blurPass = new BlurComputeTask(m_pBloomWrapperTemp, m_pBloomWrapperTemp->GetBlurWidth(), m_pBloomWrapperTemp->GetBlurHeight());

	// CopyTasks
	GraphicsPass* copyPerFrameMatricesTask = new CopyPerFrameMatricesTask();
	GraphicsPass* copyOnDemandTask = new CopyOnDemandTask();

#pragma endregion ComputeAndCopyTasks

	// Add the tasks to desired vectors so they can be used in m_pRenderer
	/* -------------------------------------------------------------- */


	/* ------------------------- CopyQueue Tasks ------------------------ */
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_PER_FRAME_MATRICES] = copyPerFrameMatricesTask;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND] = copyOnDemandTask;

	/* ------------------------- ComputeQueue Tasks ------------------------ */

	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLOOM] = blurPass;

	/* ------------------------- DirectQueue Tasks ---------------------- */
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS] = depthPrePass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY] = deferredGeometryPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_LIGHT] = deferredLightPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY] = transparentPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME] = wireFramePass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE] = outliningPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::MERGE] = mergePass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::IMGUI] = imGuiPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DOWNSAMPLE] = downSamplePass;

	/* ------------------------- DX12 Tasks ------------------------ */
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS] = blasTask;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS] = tlasTask;

	/* ------------------------- DXR Tasks ------------------------ */
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS] = reflectionPassDXR;

	// Pushback in the order of execution
	m_MainGraphicsContexts.push_back(copyOnDemandTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(copyPerFrameMatricesTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(blasTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(tlasTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(depthPrePass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(deferredGeometryPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(reflectionPassDXR->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(deferredLightPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(downSamplePass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(outliningPass->GetGraphicsContext());

	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		m_MainGraphicsContexts.push_back(wireFramePass->GetGraphicsContext());
	}
	m_MainGraphicsContexts.push_back(transparentPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(blurPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(mergePass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(copyOnDemandTask->GetGraphicsContext());

	// -------------------------------------- GUI -------------------------------------------------
	// Debug/ImGui
	m_ImGuiGraphicsContext.push_back(imGuiPass->GetGraphicsContext());
}

void Renderer::createRawBufferForLights()
{
	unsigned int rawBufferSize   = sizeof(LightHeader)		+
				MAX_DIR_LIGHTS	 * sizeof(DirectionalLight) +
				MAX_POINT_LIGHTS * sizeof(PointLight)		+
				MAX_SPOT_LIGHTS  * sizeof(SpotLight);

	// Memory on GPU
	BL_ASSERT(Light::m_pLightsRawBuffer == nullptr);
	Light::m_pLightsRawBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RawBuffer, rawBufferSize, 1, DXGI_FORMAT_UNKNOWN, L"RAW_BUFFER_LIGHTS");

	// Memory on CPU
	Light::m_pRawData = (unsigned char*)malloc(rawBufferSize);
	memset(Light::m_pRawData, 0, rawBufferSize);
}

void Renderer::setRenderTasksRenderComponents()
{
	static_cast<DepthRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::NO_DEPTH]);
	static_cast<DeferredGeometryRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::DRAW_OPAQUE]);
	static_cast<TransparentRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::DRAW_TRANSPARENT]);
}

void Renderer::prepareScene(Scene* activeScene)
{
	m_pCurrActiveScene = activeScene;
	submitUploadPerFrameData();
	submitUploadPerSceneData();

	// -------------------- DEBUG STUFF --------------------
	// Test to change m_pCamera to the shadow casting m_lights cameras
	//auto& tuple = m_Lights[LIGHT_TYPE::DIRECTIONAL_LIGHT].at(0);
	//BaseCamera* tempCam = std::get<0>(tuple)->GetCamera();
	//m_pScenePrimaryCamera = tempCam;

	BL_ASSERT_MESSAGE(!m_pScenePrimaryCamera, "No primary camera was set in scene!\n");

	m_pMousePicker->SetPrimaryCamera(m_pScenePrimaryCamera);

	setRenderTasksRenderComponents();
	setGraphicsPassesPrimaryCamera();

	// DXR, create buffers and create for the first time
	TopLevelAccelerationStructure* pTLAS = static_cast<TopLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS])->GetTLAS();

	unsigned int i = 0;
	for (RenderComponent rc : m_RayTracedRenderComponents)
	{
		pTLAS->AddInstance(rc.mc->GetModel()->m_pBLAS, *rc.tc->GetTransform()->GetWorldMatrix(), i);	// TODO: HitgroupID
		i++;
	}
	pTLAS->GenerateBuffers();
	pTLAS->SetupAccelerationStructureForBuilding(false);

	static_cast<DXRReflectionTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS])->CreateShaderBindingTable(m_RayTracedRenderComponents);
	// Currently unused
	//static_cast<DeferredLightRenderTask*>(m_GraphicsPasses[E_RENDER_TASK_TYPE::DEFERRED_GEOMETRY])->SetSceneBVHSRV(pTLAS->GetSRV());

	// PerSceneData 
	m_pCbPerSceneData->rayTracingBVH			 = pTLAS->GetRayTracingResultBuffer()->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferAlbedo			 = m_GBufferAlbedo->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferNormal			 = m_GBufferNormal->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferMaterialProperties = m_GBufferMaterialProperties->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferEmissive			 = m_GBufferEmissive->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->depth					 = m_pMainDepthStencil->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->reflectionUAV			 = m_ReflectionTexture->GetUnorderedAccessIndex();
	m_pCbPerSceneData->reflectionSRV			 = m_ReflectionTexture->GetShaderResourceHeapIndex();
}

void Renderer::submitUploadPerSceneData()
{
	// Submit CB_PER_SCENE to be uploaded to VRAM
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	BL_ASSERT(codt);
	
	BL_ASSERT(m_pCbPerScene && m_pCbPerSceneData);
	codt->SubmitBuffer(m_pCbPerScene, m_pCbPerSceneData);
}

void Renderer::submitUploadPerFrameData()
{
	// Submit dynamic-light-data to be uploaded to VRAM
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	BL_ASSERT(codt);

	// CB_PER_FRAME_STRUCT
	BL_ASSERT(m_pCbPerFrame && m_pCbPerFrameData);
	codt->SubmitBuffer(m_pCbPerFrame, m_pCbPerFrameData);

	// All lights (data and header with information)
	// Sending entire buffer (4kb as of writing this code), every frame for simplicity. Even if some lights might be static.
	BL_ASSERT(Light::m_pLightsRawBuffer && Light::m_pRawData);
	codt->SubmitBuffer(Light::m_pLightsRawBuffer, Light::m_pRawData);
}
