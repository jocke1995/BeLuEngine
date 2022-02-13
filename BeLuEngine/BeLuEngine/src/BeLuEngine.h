#ifndef ENGINE_H
#define ENGINE_H

// Miscellaneous
#include "Misc/Window.h"
#include "Misc/Timer.h"
#include "Misc/Log.h"
#include "Misc/MultiThreading/ThreadPool.h"
#include "Misc/AssetLoader.h"

// Entity Component System
#include "ECS/SceneManager.h"
#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/Lights/DirectionalLightComponent.h"
#include "ECS/Components/Lights/PointLightComponent.h"
#include "ECS/Components/Lights/SpotLightComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/SkyboxComponent.h"

// Sub-engines
#include "Renderer/Renderer.h"
#include "Renderer/Geometry/Transform.h"
#include "Renderer/Geometry/Mesh.h"
#include "Renderer/Camera/BaseCamera.h"

// Textures
#include "Renderer/Geometry/Material.h"

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
	Window* m_pWindow = nullptr;
	Timer* m_pTimer = nullptr;
	ThreadPool* m_pThreadPool = nullptr;

	Renderer* m_pRenderer = nullptr;
	SceneManager* m_pSceneManager = nullptr;
};

#endif
