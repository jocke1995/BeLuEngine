#ifndef RENDERER_H
#define RENDERER_H

// Misc
class ThreadPool;

// Renderer Engine
class BoundingBoxPool;
class DescriptorHeap;
class Mesh;
class Texture;
class Model;
class BaseCamera;

// techniques
class MousePicker;
class Bloom;

// ECS
class Scene;
class Light;

// Graphics
class GraphicsPass;

// ECS
class Entity;

// API
class IGraphicsBuffer;
class IGraphicsTexture;
class IGraphicsContext;

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
	void submitModelToGPU(Model* model);
	void submitMaterialToGPU(component::ModelComponent* mc);

	ThreadPool* m_pThreadPool = nullptr;

	// Camera
	BaseCamera* m_pScenePrimaryCamera = nullptr;

	unsigned int m_FrameCounter = 0;

	HANDLE m_ProcessHandle = nullptr;

	// -------------- Textures -------------- 
	IGraphicsTexture* m_FinalColorBuffer;
	IGraphicsTexture* m_GBufferAlbedo;
	IGraphicsTexture* m_GBufferNormal;
	IGraphicsTexture* m_GBufferMaterialProperties;
	IGraphicsTexture* m_GBufferEmissive;
	IGraphicsTexture* m_ReflectionTexture;
	
	// Depthbuffer
	IGraphicsTexture* m_pMainDepthStencil = nullptr;

	// Bloom
	Bloom* m_pBloomWrapperTemp = nullptr;

	// Picking
	MousePicker* m_pMousePicker = nullptr;
	Entity* m_pPickedEntity = nullptr;

	// Tasks
	std::vector<GraphicsPass*> m_GraphicsPasses;

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

	void setGraphicsPassesPrimaryCamera();
	void createFullScreenQuad();
	void updateMousePicker();
	void initGraphicsPasses();
	void createRawBufferForLights();
	void setRenderTasksRenderComponents();

	// Setup the whole scene
	void prepareScene(Scene* activeScene);

	// Submit cbPerSceneData to the copyQueue that updates once
	void submitUploadPerSceneData();
	// Submit cbPerFrameData to the copyQueue that updates each frame
	void submitUploadPerFrameData();

	// Contexts
	std::vector<IGraphicsContext*> m_MainGraphicsContexts;
	std::vector<IGraphicsContext*> m_ImGuiGraphicsContext;

	unsigned int m_CurrentRenderingWidth = 0;
	unsigned int m_CurrentRenderingHeight = 0;
};

#endif