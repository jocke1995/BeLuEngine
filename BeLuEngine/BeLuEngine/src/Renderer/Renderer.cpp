#include "stdafx.h"
#include "Renderer.h"

// Misc
#include "../Misc/AssetLoader.h"
#include "../Misc/MultiThreading/ThreadPool.h"
#include "../Misc/MultiThreading/Thread.h"
#include "../Misc/Window.h"
#include "../Misc/EngineStatistics.h"

// Event
#include "../Events/EventBus.h"

// Shader
#include "Shaders/DXILShaderCompiler.h"

// ImGui
#include "../ImGui/ImGuiHandler.h"

// ECS
#include "../ECS/Scene.h"
#include "../ECS/Entity.h"
#include "../ECS/Components/ModelComponent.h"
#include "../ECS/Components/BoundingBoxComponent.h"
#include "../ECS/Components/CameraComponent.h"
#include "../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../ECS/Components/Lights/PointLightComponent.h"
#include "../ECS/Components/Lights/SpotLightComponent.h"
#include "../ECS/Components/SkyboxComponent.h"

// Renderer-Engine 
#include "Geometry/Transform.h"
#include "Camera/BaseCamera.h"
#include "Geometry/Model.h"
#include "Geometry/Mesh.h"
#include "Geometry/Material.h"

// Techniques
#include "Techniques/ShadowInfo.h"
#include "Techniques/MousePicker.h"
#include "Techniques/BoundingBoxPool.h"

/* ------------------------------ RenderPasses ------------------------------ */
#include "RenderPasses/Graphics/UI/ImGuiRenderTask.h"
#include "RenderPasses/Graphics/DepthRenderTask.h"
#include "RenderPasses/Graphics/WireframeRenderTask.h"
#include "RenderPasses/Graphics/OutliningRenderTask.h"
#include "RenderPasses/Graphics/DeferredGeometryRenderTask.h"
#include "RenderPasses/Graphics/DeferredLightRenderTask.h"
#include "RenderPasses/Graphics/TransparentRenderTask.h"
#include "RenderPasses/Graphics/CopyOnDemandTask.h"
#include "RenderPasses/Graphics/SkyboxPass.h"

// Ray tracing
#include "RenderPasses/Graphics/BottomLevelRenderTask.h"
#include "RenderPasses/Graphics/TopLevelRenderTask.h"
#include "RenderPasses/Graphics/DXRReflectionTask.h"

// PostProcess
#include "RenderPasses/Graphics/PostProcess/BloomComputeTask.h"
#include "RenderPasses/Graphics/PostProcess/TonemapComputeTask.h"
/* ------------------------------ RenderPasses ------------------------------ */

// Generic API
#include "API/Interface/IGraphicsManager.h"
#include "API/Interface/IGraphicsBuffer.h"
#include "API/Interface/IGraphicsTexture.h"

// Generic API (Raytracing)
#include "API/Interface/RayTracing/IBottomLevelAS.h"
#include "API/Interface/RayTracing/ITopLevelAS.h"

Renderer::Renderer()
{
	//EventBus::GetInstance().Subscribe(this, &Renderer::toggleFullscreen);
	m_GraphicsPasses.resize(E_GRAPHICS_PASS_TYPE::NR_OF_GRAPHICS_PASSES);
	for (GraphicsPass* pass : m_GraphicsPasses)
		pass = nullptr;

	// Processinfo
	// Create handle to process
	DWORD currentProcessID = GetCurrentProcessId();
	m_ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, currentProcessID);

	if (m_ProcessHandle == nullptr)
	{
		BL_LOG_CRITICAL("Failed to create handle to process\n");
	}

	// Subscribe to events
	EventBus::GetInstance().Subscribe(this, &Renderer::advanceTextureToVisualize);
}

Renderer& Renderer::GetInstance()
{
	static Renderer instance;
	return instance;
}

Renderer::~Renderer()
{
	// Unsubscrive to events
	EventBus::GetInstance().Unsubscribe(this, &Renderer::advanceTextureToVisualize);
}

void Renderer::deleteRenderer()
{
	Log::Print("----------------------------  Deleting Renderer  ----------------------------------\n");

	BL_SAFE_DELETE(m_pFullScreenQuad);

	// Common Graphics Resources
	cleanupCommonBuffers();

	for (GraphicsPass* graphicsPass : m_GraphicsPasses)
		BL_SAFE_DELETE(graphicsPass);

	BL_SAFE_DELETE(m_pMousePicker);

	// Constant Buffers
	BL_SAFE_DELETE(m_GraphicsResources.cbPerScene);
	BL_SAFE_DELETE(m_GraphicsResources.cbPerFrame);

	// ConstantBufferDataHolders
	BL_SAFE_DELETE(m_pCbPerSceneData);
	BL_SAFE_DELETE(m_pCbPerFrameData);

	BL_SAFE_DELETE(Light::m_pLightsRawBuffer);
	free(Light::m_pRawData);
}

void Renderer::Init(E_RESOLUTION_TYPES resolution)
{
	onResizedResolution(resolution);

	// Picking
	m_pMousePicker = new MousePicker();
	
	DXILShaderCompiler::Get()->Init();

	// FullScreenQuad
	createFullScreenQuad();

	// Init Assetloader (Note: This should be free to use before this line, before it wasn't)
	AssetLoader* al = AssetLoader::Get();

	// Init BoundingBoxPool
	BoundingBoxPool::Get();

	// Allocate memory for cbPerScene
	m_GraphicsResources.cbPerScene = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(CB_PER_SCENE_STRUCT), 1, BL_FORMAT_UNKNOWN, L"CB_PER_SCENE");
	m_pCbPerSceneData = new CB_PER_SCENE_STRUCT();

	// Allocate memory for cbPerFrame
	m_GraphicsResources.cbPerFrame = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(CB_PER_FRAME_STRUCT), 1, BL_FORMAT_UNKNOWN, L"CB_PER_FRAME");
	m_pCbPerFrameData = new CB_PER_FRAME_STRUCT();

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

	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	BL_ASSERT(codt);
	
	TODO("Using this vector only because all objects are stored here.. Figure out a better solution in the future")
	// Submit Transforms (Currently using the rayTracingVector cause all objects are in that one.
	for (const RenderComponent& rc : m_RayTracedRenderComponents)
	{
		Transform* t = rc.tc->GetTransform();
		BL_ASSERT(t);

		t->UpdateWorldWVP(*m_pScenePrimaryCamera->GetViewProjectionTranposed());

		IGraphicsBuffer* matricesBuffer = t->GetConstantBuffer();
		codt->SubmitBuffer(matricesBuffer, t->GetMatricesPerObjectData());
	}

	// Picking
	updateMousePicker();

	// Update the resultBuffer of DXR if needed
	ITopLevelAS* pTLAS = static_cast<TopLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS])->GetTopLevelAS();
	pTLAS->Reset();

	unsigned int i = 0;
	for (RenderComponent rc : m_RayTracedRenderComponents)
	{
		bool giveShadow = rc.mc->GetDrawFlag() & F_DRAW_FLAGS::GIVE_SHADOW;
		pTLAS->AddInstance(rc.mc->GetModel()->m_pBLAS, *rc.tc->GetTransform()->GetWorldMatrix(), i, giveShadow);
		i++;
	}

	// This function is smart enough to only create a new buffer if it needs to.
	// If it does, the descriptor to the resultBuffer needs to be updated
	if (pTLAS->CreateResultBuffer())
	{
		submitCbPerSceneData(pTLAS);
	}

	setDataForRendering();
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
	setDataForRendering();
}

void Renderer::Execute()
{
	IGraphicsManager* graphicsManager = IGraphicsManager::GetBaseInstance();
	graphicsManager->Begin();
	
	ThreadPool& threadPool = ThreadPool::GetInstance();
	auto executeGraphicsPassLambda = [this, &threadPool](E_GRAPHICS_PASS_TYPE graphicsPass)
	{
		// For easier debugging purposes
		if (SINGLE_THREADED_RENDERER == true)
		{
			m_GraphicsPasses[graphicsPass]->Execute();
		}
		else
		{
			threadPool.AddTask(m_GraphicsPasses[graphicsPass]);
		}
	};

	auto waitForCompletionLambda = [this, &threadPool](F_THREAD_FLAGS threadFlag)
	{
		// For easier debugging purposes
		if (SINGLE_THREADED_RENDERER == true)
		{
			// Do nothing, everything is allready completed if we reach this point using a single threaded commandList recording
		}
		else
		{
			threadPool.WaitForThreads(threadFlag);
		}
	};

	/* --------------------- Record command lists (Heavy ones are added first!) --------------------- */
	/* ----------------------- Note: NOT EXECUTED IN THE ORDER THEY ARE HERE! ----------------------- */

	// Depth pre-pass
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS);

	// Geometry pass
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY);

	// Blend
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::OPACITY);

	// Blurring for bloom
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::POSTPROCESS_BLOOM);

	// DXR Reflections
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::REFLECTIONS);

	// Update TLAS
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::TLAS);

	// Copy on demand
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND);

	// Update BLASes if any...
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::BLAS);

	// Light pass
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::DEFERRED_LIGHT);

	// Outlining, if an object is picked
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::OUTLINE);

	// Merge 
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::POSTPROCESS_TONEMAP);

	// Skybox 
	executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::SKYBOX);
	
	TODO("This will be moved into an editor-Mode only")
	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */
	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		executeGraphicsPassLambda(E_GRAPHICS_PASS_TYPE::WIREFRAME);
	}

	/* ----------------------------- DEVELOPERMODE CommandLists ----------------------------- */

	// Waits for context-Threads to complete if we're using multithreaded recording of contexts
	waitForCompletionLambda(F_THREAD_FLAGS::GRAPHICS);

	graphicsManager->ExecuteGraphicsContexts(m_MainGraphicsContexts, m_MainGraphicsContexts.size());
	/* --------------------------------------------------------------- */

	TODO("Wrap this in an editor-Mode only. This task has to be run on the mainthread!");
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::IMGUI]->Execute();

	graphicsManager->ExecuteGraphicsContexts(m_ImGuiGraphicsContext, m_ImGuiGraphicsContext.size());

	/*------------------- Present -------------------*/
	TODO("This overrides the editorStuff.. this texture should be copied to the finalColorTexture instead before we draw the editor, so it isn't removed");
	graphicsManager->SyncAndPresent(m_CurrentTextureToVisualize);

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
		t->m_pConstantBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::ConstantBuffer, sizeof(DirectX::XMMATRIX), 2, BL_FORMAT_UNKNOWN, L"Transform");

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

		static_cast<SkyboxPass*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::SKYBOX])->SetCamera(m_pScenePrimaryCamera);
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

void Renderer::InitSkyboxComponent(component::SkyboxComponent* component)
{
	static_cast<SkyboxPass*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::SKYBOX])->SetSkybox(component);

	IGraphicsTexture* skyboxTexture = component->GetSkyboxTexture();
	m_pCbPerSceneData->skybox = skyboxTexture->GetShaderResourceHeapIndex();

	// Submit cube
	TODO("Use the internal MeshFactory to create simple meshes and just get it from there.");
	component->m_pCube = AssetLoader::Get()->LoadModel(L"../Vendor/Assets/Models/CubePBR/cube.obj");

	Mesh* mesh = component->m_pCube->GetMeshAt(0);

	// Submit Mesh
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	codt->SubmitBuffer(mesh->GetVertexBuffer(), mesh->GetVertices()->data());
	codt->SubmitBuffer(mesh->GetIndexBuffer(), mesh->GetIndices()->data());

	AssetLoader::Get()->m_LoadedModels.at(component->m_pCube->GetPath()).first = true;

	// Submit model to BLAS
	static_cast<BottomLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS])->SubmitBLAS(component->m_pCube->m_pBLAS);


	// Submit SkyboxTexture
	if (AssetLoader::Get()->IsTextureLoadedOnGpu(skyboxTexture) == true)
	{
		return;
	}

	codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	codt->SubmitTexture(skyboxTexture);

	AssetLoader::Get()->m_LoadedTextures.at(skyboxTexture->GetPath()).first = true;
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

	// Delete constantBuffer for matrices
	if (tc->GetTransform()->m_pConstantBuffer != nullptr)
	{
		delete tc->GetTransform()->m_pConstantBuffer;
		tc->GetTransform()->m_pConstantBuffer = nullptr;
	}

	// Update Render Tasks components (forward the change in renderComponents)
	setDataForRendering();
}

void Renderer::UnInitDirectionalLightComponent(component::DirectionalLightComponent* component)
{
	TODO("Remove from rawBuffer");
}

void Renderer::UnInitPointLightComponent(component::PointLightComponent* component)
{
	TODO("Remove from rawBuffer");
}

void Renderer::UnInitSpotLightComponent(component::SpotLightComponent* component)
{
	TODO("Remove from rawBuffer");
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

void Renderer::UnInitSkyboxComponent(component::SkyboxComponent* component)
{
	static_cast<SkyboxPass*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::SKYBOX])->SetSkybox(nullptr);

	m_pCbPerSceneData->skybox = -1;
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

void Renderer::onResizedResolution(const E_RESOLUTION_TYPES newResolution)
{
	BL_ASSERT(m_CurrentResolutionType != newResolution);
	m_CurrentResolutionType = newResolution;

	GetWidthHeightResolution(m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight, newResolution);

	// Delete the old buffers if they exist
	cleanupCommonBuffers();

	// Main color renderTarget (used until the swapchain RT is drawn to)
	m_GraphicsResources.finalColorBuffer = IGraphicsTexture::Create();
	m_GraphicsResources.finalColorBuffer->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget | F_TEXTURE_USAGE::UnorderedAccess,
		L"FinalColorbuffer");
	m_CurrentTextureToVisualize = m_GraphicsResources.finalColorBuffer;

	// GBufferAlbedo
	m_GraphicsResources.gBufferAlbedo = IGraphicsTexture::Create();
	m_GraphicsResources.gBufferAlbedo->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R8G8B8A8_UNORM,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferAlbedo");

	// Normal
	m_GraphicsResources.gBufferNormal = IGraphicsTexture::Create();
	m_GraphicsResources.gBufferNormal->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R16G16B16A16_FLOAT, // TODO: Does it needs to be this high..?
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferNormal");

	// Material Properties (Roughness, Metallic, glow..
	m_GraphicsResources.gBufferMaterialProperties = IGraphicsTexture::Create();
	m_GraphicsResources.gBufferMaterialProperties->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R8G8B8A8_UNORM,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferMaterials");

	// Emissive Color
	m_GraphicsResources.gBufferEmissive = IGraphicsTexture::Create();
	m_GraphicsResources.gBufferEmissive->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::RenderTarget,
		L"gBufferEmissive");

	// ReflectionTexture
	m_GraphicsResources.reflectionTexture = IGraphicsTexture::Create();
	m_GraphicsResources.reflectionTexture->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_R16G16B16A16_FLOAT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::UnorderedAccess,
		L"ReflectionTexture");

	// DepthBuffer
	m_GraphicsResources.mainDepthStencil = IGraphicsTexture::Create();
	m_GraphicsResources.mainDepthStencil->CreateTexture2D(
		m_GraphicsResources.renderWidth, m_GraphicsResources.renderHeight,
		BL_FORMAT_D24_UNORM_S8_UINT,
		F_TEXTURE_USAGE::ShaderResource | F_TEXTURE_USAGE::DepthStencil,
		L"MainDepthStencilBuffer");

	// Resize bloomBuffer
	// Adding an extra layer of safety because in the future I might wanna remove entire passes from memory depending on if they're active or not
	BloomComputePass* bloomPass = static_cast<BloomComputePass*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::POSTPROCESS_BLOOM]);
	if(bloomPass)
		bloomPass->onResizeInternalBuffers();
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
	// Acceleration Structures
	GraphicsPass* blasTask = new BottomLevelRenderTask();
	GraphicsPass* tlasTask = new TopLevelRenderTask();

	// DXR
	GraphicsPass* reflectionPassDXR = new DXRReflectionTask();
	reflectionPassDXR->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);

	// DepthPrePass
	GraphicsPass* depthPrePass = new DepthRenderTask();

	// Deferred Geometry Pass
	GraphicsPass* deferredGeometryPass = new DeferredGeometryRenderTask();

	// Deferred Light Pass
	GraphicsPass* deferredLightPass = new DeferredLightRenderTask(m_pFullScreenQuad);
	deferredLightPass->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);

	GraphicsPass* outliningPass = new OutliningRenderTask();

	GraphicsPass* transparentPass = new TransparentRenderTask();
	transparentPass->AddGraphicsBuffer("rawBufferLights", Light::m_pLightsRawBuffer);

	GraphicsPass* wireFramePass = new WireframeRenderTask();

	// Post Processing
	GraphicsPass* bloomPass = new BloomComputePass();
	GraphicsPass* tonemapPass = new TonemapComputeTask();

	// Skybox
	GraphicsPass* skyboxPass = new SkyboxPass();

	// LazyCopy
	GraphicsPass* copyOnDemandTask = new CopyOnDemandTask();

	// UI
	GraphicsPass* imGuiPass = new ImGuiRenderTask();

	// Add the tasks to desired vectors so they can be used in m_pRenderer
	/* -------------------------------------------------------------- */


	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND] = copyOnDemandTask;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::BLAS] = blasTask;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS] = tlasTask;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS] = depthPrePass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY] = deferredGeometryPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS] = reflectionPassDXR;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_LIGHT] = deferredLightPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE] = outliningPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::WIREFRAME] = wireFramePass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY] = transparentPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::POSTPROCESS_BLOOM] = bloomPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::POSTPROCESS_TONEMAP] = tonemapPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::SKYBOX] = skyboxPass;
	m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::IMGUI] = imGuiPass;

	// Pushback in the order of execution
	m_MainGraphicsContexts.push_back(copyOnDemandTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(blasTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(tlasTask->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(depthPrePass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(deferredGeometryPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(reflectionPassDXR->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(deferredLightPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(outliningPass->GetGraphicsContext());

	if (DEVELOPERMODE_DRAWBOUNDINGBOX == true)
	{
		m_MainGraphicsContexts.push_back(wireFramePass->GetGraphicsContext());
	}
	m_MainGraphicsContexts.push_back(transparentPass->GetGraphicsContext());

	// PostProcess
	m_MainGraphicsContexts.push_back(bloomPass->GetGraphicsContext());
	m_MainGraphicsContexts.push_back(tonemapPass->GetGraphicsContext());

	// Skybox
	m_MainGraphicsContexts.push_back(skyboxPass->GetGraphicsContext());
	
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
	Light::m_pLightsRawBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RawBuffer, rawBufferSize, 1, BL_FORMAT_UNKNOWN, L"RAW_BUFFER_LIGHTS");

	// Memory on CPU
	Light::m_pRawData = (unsigned char*)malloc(rawBufferSize);
	memset(Light::m_pRawData, 0, rawBufferSize);
}

void Renderer::setDataForRendering()
{
	// --------------------------- RenderComponents --------------------------- //
	static_cast<DepthRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEPTH_PRE_PASS])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::NO_DEPTH]);
	static_cast<DeferredGeometryRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::DEFERRED_GEOMETRY])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::DRAW_OPAQUE]);
	static_cast<TransparentRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OPACITY])->SetRenderComponents(m_RenderComponents[F_DRAW_FLAGS::DRAW_TRANSPARENT]);

	// DXR
	static_cast<TopLevelRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::TLAS])->SetRenderComponents(m_RayTracedRenderComponents);
	static_cast<DXRReflectionTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::REFLECTIONS])->SetRenderComponents(m_RayTracedRenderComponents);
	// --------------------------- RenderComponents --------------------------- //

	// Camera
	static_cast<OutliningRenderTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::OUTLINE])->SetCamera(m_pScenePrimaryCamera);
}

void Renderer::setupNewScene(Scene* activeScene)
{
	m_pCurrActiveScene = activeScene;

	BL_ASSERT_MESSAGE(m_pScenePrimaryCamera, "No primary camera was set in scene!\n");
	m_pMousePicker->SetPrimaryCamera(m_pScenePrimaryCamera);

	submitUploadPerFrameData();
}

void Renderer::submitUploadPerFrameData()
{
	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	BL_ASSERT(codt);

	// CB_PER_FRAME_STRUCT
	BL_ASSERT(m_GraphicsResources.cbPerFrame && m_pCbPerFrameData);
	codt->SubmitBuffer(m_GraphicsResources.cbPerFrame, m_pCbPerFrameData);

	// All lights (data and header with information)
	// Sending entire buffer (4kb as of writing this code), every frame for simplicity. Even if some lights might be static.
	BL_ASSERT(Light::m_pLightsRawBuffer && Light::m_pRawData);
	codt->SubmitBuffer(Light::m_pLightsRawBuffer, Light::m_pRawData);
}

void Renderer::submitCbPerSceneData(ITopLevelAS* pTLAS)
{
	BL_ASSERT(pTLAS);

	// Make sure the indices are correct 
	m_pCbPerSceneData->rayTracingBVH = pTLAS->GetRayTracingResultBuffer()->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferAlbedo = m_GraphicsResources.gBufferAlbedo->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferNormal = m_GraphicsResources.gBufferNormal->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferMaterialProperties = m_GraphicsResources.gBufferMaterialProperties->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->gBufferEmissive = m_GraphicsResources.gBufferEmissive->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->reflectionTextureSRV = m_GraphicsResources.reflectionTexture->GetShaderResourceHeapIndex();
	m_pCbPerSceneData->reflectionTextureUAV = m_GraphicsResources.reflectionTexture->GetUnorderedAccessIndex();
	m_pCbPerSceneData->depth = m_GraphicsResources.mainDepthStencil->GetShaderResourceHeapIndex();

	// Update the rendererWidth and renderingHeight as well in case it was changed
	m_pCbPerSceneData->renderingWidth = m_GraphicsResources.renderWidth;
	m_pCbPerSceneData->renderingHeight = m_GraphicsResources.renderHeight;

	CopyOnDemandTask* codt = static_cast<CopyOnDemandTask*>(m_GraphicsPasses[E_GRAPHICS_PASS_TYPE::COPY_ON_DEMAND]);
	BL_ASSERT(codt);
	BL_ASSERT(m_GraphicsResources.cbPerScene && m_pCbPerSceneData);
	codt->SubmitBuffer(m_GraphicsResources.cbPerScene, m_pCbPerSceneData);
}

void Renderer::cleanupCommonBuffers()
{
	BL_SAFE_DELETE(m_GraphicsResources.finalColorBuffer);
	BL_SAFE_DELETE(m_GraphicsResources.gBufferAlbedo);
	BL_SAFE_DELETE(m_GraphicsResources.gBufferNormal);
	BL_SAFE_DELETE(m_GraphicsResources.gBufferMaterialProperties);
	BL_SAFE_DELETE(m_GraphicsResources.gBufferEmissive);
	BL_SAFE_DELETE(m_GraphicsResources.reflectionTexture);
	BL_SAFE_DELETE(m_GraphicsResources.mainDepthStencil);
}

void Renderer::advanceTextureToVisualize(VisualizeTexture* event)
{
	static unsigned int moduloCounter = 0;

	switch (moduloCounter)
	{
		TODO("This won't work anymore because they are not the same format.. need to do a separate pass to visualize these!");
		//case -1:
		//	m_CurrentTextureToVisualize = m_GraphicsResources.gBufferAlbedo;
		//	break;
		//case -1:
		//	m_CurrentTextureToVisualize = m_GraphicsResources.gBufferMaterialProperties;
		//	break;
		//case -1:
		//	m_CurrentTextureToVisualize = m_GraphicsResources.depth...;
		//	break;
		//case -1:
		//	m_CurrentTextureToVisualize = m_GraphicsResources.motionVectors;
		//	break;

		case 0:
			m_CurrentTextureToVisualize = m_GraphicsResources.gBufferNormal;
			break;
		case 1:
			m_CurrentTextureToVisualize = m_GraphicsResources.reflectionTexture;
			break;
		case 2:
			m_CurrentTextureToVisualize = m_GraphicsResources.gBufferEmissive;
			break;
		case 3:
			m_CurrentTextureToVisualize = m_GraphicsResources.finalColorBuffer;
			break;
	}

	moduloCounter++;
	moduloCounter %= 4;
}
