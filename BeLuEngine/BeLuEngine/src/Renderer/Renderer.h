#ifndef RENDERER_H
#define RENDERER_H

// Misc
class ThreadPool;

// Renderer Engine
class SwapChain;
class BoundingBoxPool;
class DescriptorHeap;
class Mesh;
class Texture;
class Model;
class Resource;

// GPU Resources
class ShaderResource;
class UnorderedAccess;
class DepthStencil;
class RenderTarget;
class Resource;

// Descriptors
class RenderTargetView;
class ShaderResourceView;
class UnorderedAccessView;
class ConstantBufferView;
class DepthStencilView;


// techniques
class MousePicker;
class Bloom;

// ECS
class Scene;
class Light;

// Graphics
#include "DX12Tasks/RenderTask.h"
class BaseCamera;

// Copy
class CopyTask;

// Compute
class ComputeTask;

// DXR
class DXRTask;

// ECS
class Entity;

// API
class IGraphicsBuffer;

TODO("Replace with GraphicsContext");
struct ID3D12CommandList;

namespace component
{
	class ModelComponent;
	class TransformComponent;
	class CameraComponent;
	class BoundingBoxComponent;
	class DirectionalLightComponent;
	class PointLightComponent;
	class SpotLightComponent;
}

struct RenderComponent;

// Events
struct WindowChange;
struct WindowSettingChange;

class Renderer
{
public:
	static Renderer& GetInstance();
	virtual ~Renderer();

	// PickedEntity
	Entity* const GetPickedEntity() const;
	// Scene
	Scene* const GetActiveScene() const;

	// Call once
	void InitD3D12(HWND hwnd, unsigned int width, unsigned int height, HINSTANCE hInstance, ThreadPool* threadPool);

	// Call each frame
	void Update(double dt);
	void SortObjects();

	// Call one of these (Single threaded-version is most used for debugging purposes)
	void ExecuteMT();
	void ExecuteST();

	// Render inits, these functions are called by respective components through SetScene to prepare for drawing
	void InitModelComponent(component::ModelComponent* component);
	void InitDirectionalLightComponent(component::DirectionalLightComponent* component);
	void InitPointLightComponent(component::PointLightComponent* component);
	void InitSpotLightComponent(component::SpotLightComponent* component);
	void InitCameraComponent(component::CameraComponent* component);
	void InitBoundingBoxComponent(component::BoundingBoxComponent* component);

	void UnInitModelComponent(component::ModelComponent* component);
	void UnInitDirectionalLightComponent(component::DirectionalLightComponent* component);
	void UnInitPointLightComponent(component::PointLightComponent* component);
	void UnInitSpotLightComponent(component::SpotLightComponent* component);
	void UnInitCameraComponent(component::CameraComponent* component);
	void UnInitBoundingBoxComponent(component::BoundingBoxComponent* component);

	void OnResetScene();

private:
	friend class BeLuEngine;
	friend class SceneManager;
	friend class ImGuiHandler;
	friend class Material;
	friend class component::ModelComponent;

	Renderer();

	// For control of safe release of DirectX resources
	void deleteRenderer();

	// SubmitToCodt functions
	void submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void submitModelToGPU(Model* model);
	void submitSlotInfoRawBufferToGPU(component::ModelComponent* mc);
	void submitMaterialToGPU(component::ModelComponent* mc);
	void submitMeshToCodt(Mesh* mesh);
	void submitTextureToCodt(Texture* texture);

	ThreadPool* m_pThreadPool = nullptr;

	// Camera
	BaseCamera* m_pScenePrimaryCamera = nullptr;

	unsigned int m_FrameCounter = 0;

	HANDLE m_ProcessHandle = nullptr;

	// -------------- RenderTargets -------------- 
	std::pair<RenderTarget*, ShaderResourceView*> m_FinalColorBuffer;
	std::pair<RenderTarget*, ShaderResourceView*> m_GBufferAlbedo;
	std::pair<RenderTarget*, ShaderResourceView*> m_GBufferNormal;
	std::pair<RenderTarget*, ShaderResourceView*> m_GBufferMaterialProperties;
	std::pair<RenderTarget*, ShaderResourceView*> m_GBufferEmissive;

	// -------------- RenderTargets -------------- 
	Resource_UAV_SRV m_ReflectionTexture;
	
	// Bloom (includes rtv, uav and srv)
	Bloom* m_pBloomResources = nullptr;

	// Depthbuffer
	DepthStencil* m_pMainDepthStencil = nullptr;

	// -------------- RenderTargets -------------- 
	// Picking
	MousePicker* m_pMousePicker = nullptr;
	Entity* m_pPickedEntity = nullptr;

	// Tasks
	std::vector<ComputeTask*> m_ComputeTasks;
	std::vector<CopyTask*>    m_CopyTasks;
	std::vector<RenderTask*>  m_RenderTasks;
	std::vector<DX12Task*>	  m_DX12Tasks;
	std::vector<DXRTask*>	  m_DXRTasks;

	Mesh* m_pFullScreenQuad = nullptr;

	// Group of components that's needed for rendering:
	std::map<F_DRAW_FLAGS, std::vector<RenderComponent>> m_RenderComponents;
	std::vector<RenderComponent> m_RayTracedRenderComponents;
	std::vector<component::BoundingBoxComponent*> m_BoundingBoxesToBePicked;

	// Current scene to be drawn
	Scene* m_pCurrActiveScene = nullptr;
	CB_PER_SCENE_STRUCT* m_pCbPerSceneData = nullptr;
	IGraphicsBuffer* m_pCbPerScene = nullptr;

	// update per frame
	CB_PER_FRAME_STRUCT* m_pCbPerFrameData = nullptr;
	IGraphicsBuffer* m_pCbPerFrame = nullptr;

	void setRenderTasksPrimaryCamera();
	void createMainDSV();
	void createFullScreenQuad();
	void updateMousePicker();
	void initRenderTasks();
	void createRawBufferForLights();
	void setRenderTasksRenderComponents();

	// Setup the whole scene
	void prepareScene(Scene* activeScene);

	// Submit cbPerSceneData to the copyQueue that updates once
	void submitUploadPerSceneData();
	// Submit cbPerFrameData to the copyQueue that updates each frame
	void submitUploadPerFrameData();

	//void toggleFullscreen(WindowChange* event);

	// CommandInterface
	std::vector<ID3D12CommandList*> m_DirectCommandLists[NUM_SWAP_BUFFERS];
	std::vector<ID3D12CommandList*> m_ImGuiCommandLists[NUM_SWAP_BUFFERS];

	unsigned int m_CurrentRenderingWidth = 0;
	unsigned int m_CurrentRenderingHeight = 0;
};

#endif