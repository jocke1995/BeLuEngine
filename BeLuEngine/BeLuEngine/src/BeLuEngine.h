#ifndef ENGINE_H
#define ENGINE_H

// Miscellaneous
#include "Misc/Window.h"
#include "Misc/Timer.h"
#include "Misc/MultiThreading/ThreadPool.h"
#include "Misc/AssetLoader.h"

// Entity Component System
#include "ECS/SceneManager.h"
#include "ECS/Entity.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/Lights/DirectionalLightComponent.h"
#include "ECS/Components/Lights/PointLightComponent.h"
#include "ECS/Components/Lights/SpotLightComponent.h"
#include "ECS/Components/CameraComponent.h"

// Sub-engines
#include "Renderer/Renderer.h"
#include "Renderer/Transform.h"
#include "Renderer/Mesh.h"
#include "Renderer/Camera/BaseCamera.h"

// Textures
#include "Renderer/Material.h"
#include "Renderer/Texture/TextureCubeMap.h"
#include "Renderer/Texture/Texture2DGUI.h"

// Event-handling
#include "Events/EventBus.h"

// Input
#include "Input/Input.h"

class BeLuEngine
{
public:
	BeLuEngine();
	~BeLuEngine();

	void Init(HINSTANCE hInstance, int nCmdShow);

	Window* const GetWindow() const;
	Timer* const GetTimer() const;
	ThreadPool* const GetThreadPool() const;

	Renderer* const GetRenderer() const;
	SceneManager* const GetSceneHandler() const;

private:
	friend class ModelComponent;

	Window* m_pWindow = nullptr;
	Timer* m_pTimer = nullptr;
	ThreadPool* m_pThreadPool = nullptr;

	Renderer* m_pRenderer = nullptr;
	SceneManager* m_pSceneManager = nullptr;
};

#endif
