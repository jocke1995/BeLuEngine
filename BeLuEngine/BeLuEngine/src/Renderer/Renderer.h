#ifndef RENDERER_H
#define RENDERER_H

// USE_NSIGHT_AFTERMATH
#if defined(USE_NSIGHT_AFTERMATH)
	#include "aftermath/GFSDK_Aftermath.h"
	#include "../Misc/NvidiaAftermath/NsightAftermathGpuCrashTracker.h"
#endif

// Misc
class ThreadPool;
class Window;

// Renderer Engine
class SwapChain;
class BoundingBoxPool;
class DescriptorHeap;
class Mesh;
class Texture;
class Model;
class Resource;

// GPU Resources
class ConstantBuffer;
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

// Enums
enum E_COMMAND_INTERFACE_TYPE;
enum class E_DESCRIPTOR_HEAP_TYPE;

// techniques
class ShadowInfo;
class MousePicker;
class Bloom;

// ECS
class Scene;
class Light;

// Graphics
#include "DX12Tasks/RenderTask.h"
class WireframeRenderTask;
class OutliningRenderTask;
class BaseCamera;
class Material;

// Copy
class CopyTask;

// Compute
class ComputeTask;

// DXR
class DXRTask;

// DX12 Forward Declarations
struct ID3D12CommandQueue;
struct ID3D12CommandList;
struct ID3D12Fence1;
struct ID3D12Device5;
struct IDXGIAdapter4;

// ECS
class Entity;
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

	Window* const GetWindow() const;

	// Call once
	void InitD3D12(Window* window, HINSTANCE hInstance, ThreadPool* threadPool);

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
	Renderer();

	// For control of safe release of DirectX resources
	void deleteRenderer();

	// SubmitToCodt functions
	void submitToCodt(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void submitModelToGPU(Model* model);
	void submitMaterialToGPU(component::ModelComponent* mc);
	void submitMaterialDataToGPU(Material* mat);
	void submitMeshToCodt(Mesh* mesh);
	void submitTextureToCodt(Texture* texture);

	//SubmitToCpft functions
	void submitToCpft(std::tuple<Resource*, Resource*, const void*>* Upload_Default_Data);
	void clearSpecificCpft(Resource* upload);

	DescriptorHeap* getCBVSRVUAVdHeap() const;

	ThreadPool* m_pThreadPool = nullptr;

	// Camera
	BaseCamera* m_pScenePrimaryCamera = nullptr;

	unsigned int m_FrameCounter = 0;

	// Window
	Window* m_pWindow;

	// Device
	ID3D12Device5* m_pDevice5 = nullptr;

	// Adapters used for getting VRAM and RAM
	IDXGIAdapter4* m_pAdapter4 = nullptr;
	HANDLE m_ProcessHandle = nullptr;

	// CommandQueues
	std::map<E_COMMAND_INTERFACE_TYPE, ID3D12CommandQueue*> m_CommandQueues;

	// -------------- RenderTargets -------------- 
	std::pair<RenderTarget*, ShaderResourceView*> m_pMainColorBuffer;

	// Swapchain (inheriting from 'RenderTarget')
	SwapChain* m_pSwapChain = nullptr;
	
	// Bloom (includes rtv, uav and srv)
	Bloom* m_pBloomResources = nullptr;

	// Depthbuffer
	DepthStencil* m_pMainDepthStencil = nullptr;

	ID3D12RootSignature* m_pGlobalRootSig = nullptr;

	// Picking
	MousePicker* m_pMousePicker = nullptr;
	Entity* m_pPickedEntity = nullptr;

	// Tasks
	std::vector<ComputeTask*> m_ComputeTasks;
	std::vector<CopyTask*>    m_CopyTasks;
	std::vector<RenderTask*>  m_RenderTasks;
	std::vector<DXRTask*>	  m_DXRTasks;

	Mesh* m_pFullScreenQuad = nullptr;

	// Group of components that's needed for rendering:
	std::map<F_DRAW_FLAGS, std::vector<RenderComponent>> m_RenderComponents;
	std::vector<component::BoundingBoxComponent*> m_BoundingBoxesToBePicked;

	// Current scene to be drawn
	Scene* m_pCurrActiveScene = nullptr;
	CB_PER_SCENE_STRUCT* m_pCbPerSceneData = nullptr;
	ConstantBuffer* m_pCbPerScene = nullptr;

	// update per frame
	CB_PER_FRAME_STRUCT* m_pCbPerFrameData = nullptr;
	ConstantBuffer* m_pCbPerFrame = nullptr;

	// Commandlists holders
	std::vector<ID3D12CommandList*> m_DirectCommandLists[NUM_SWAP_BUFFERS];
	std::vector<ID3D12CommandList*> m_ImGuiCommandLists[NUM_SWAP_BUFFERS];

	// DescriptorHeaps
	std::map<E_DESCRIPTOR_HEAP_TYPE, DescriptorHeap*> m_DescriptorHeaps = {};

	// Fences
	HANDLE m_EventHandle = nullptr;
	ID3D12Fence1* m_pFenceFrame = nullptr;
	UINT64 m_FenceFrameValue = 0;

	void setRenderTasksPrimaryCamera();
	bool createDevice();
	void createCommandQueues();
	void createSwapChain();
	void createMainDSV();
	void createRootSignature();
	void createFullScreenQuad();
	void updateMousePicker();
	void initRenderTasks();
	void createRawBufferForLights();
	void setRenderTasksRenderComponents();
	void createDescriptorHeaps();
	void createFences();
	void waitForFrame(unsigned int framesToBeAhead = NUM_SWAP_BUFFERS - 1);
	void waitForGPU();

	// Setup the whole scene
	void prepareScene(Scene* activeScene);

	// Submit cbPerSceneData to the copyQueue that updates once
	void submitUploadPerSceneData();
	// Submit cbPerFrameData to the copyQueue that updates each frame
	void submitUploadPerFrameData();

	void toggleFullscreen(WindowChange* event);

	SwapChain* getSwapChain() const;

// USE_NSIGHT_AFTERMATH
#if defined(USE_NSIGHT_AFTERMATH)
	int32_t* m_pAfterMathContextHandle = nullptr;
	GpuCrashTracker m_GpuCrashTracker = {};
#endif

};

#endif