#include "stdafx.h"
#include "SceneManager.h"

// Misc
#include "../Misc/AssetLoader.h"
#include "../Events/EventBus.h"

// Renderer
#include "../Renderer/Renderer.h"
#include "../Renderer/CommandInterface.h"
#include "../Renderer/Geometry/Mesh.h"

// CopyTasks
#include "../Renderer/DX12Tasks/CopyOnDemandTask.h"

// RenderTasks
#include "../Renderer/DX12Tasks/WireframeRenderTask.h"

// Techniques
#include "../Renderer/Techniques/BoundingBoxPool.h"
#include "../Renderer/Techniques/ShadowInfo.h"

// ECS
#include "../ECS/Entity.h"
#include "../ECS/Scene.h"

// Components
#include "../ECS/Components/Lights/DirectionalLightComponent.h"
#include "../ECS/Components/Lights/PointLightComponent.h"
#include "../ECS/Components/Lights/SpotLightComponent.h"

SceneManager::SceneManager()
{
	
}

SceneManager& SceneManager::GetInstance()
{
	static SceneManager instance;
	return instance;
}

SceneManager::~SceneManager()
{
}

void SceneManager::deleteSceneManager()
{
	for (auto pair : m_Scenes)
	{
		delete pair.second;
	}

	m_Scenes.clear();
}

void SceneManager::Update(double dt)
{
	// Update scene
	m_pActiveScene->Update(this, dt);

	Renderer::GetInstance().Update(dt);
}

Scene* SceneManager::CreateScene(std::string sceneName)
{
    if (sceneExists(sceneName))
    {
		BL_LOG_CRITICAL("A scene with the name: \'%s\' already exists.\n", sceneName.c_str());
        return nullptr;
    }

    // Create Scene and return it
    m_Scenes[sceneName] = new Scene(sceneName);
    return m_Scenes[sceneName];
}

Scene* SceneManager::GetActiveScene()
{
	return m_pActiveScene;
}

Scene* SceneManager::GetScene(std::string sceneName) const
{
    if (sceneExists(sceneName))
    {
        return m_Scenes.at(sceneName);
    }
	
	BL_LOG_CRITICAL("No Scene with name: \'%s\' was found.\n", sceneName.c_str());
    return nullptr;
}

void SceneManager::RemoveEntity(Entity* entity, Scene* scene)
{
	entity->OnUnInitScene();

	// Remove from the scene
	scene->RemoveEntity(entity->GetName());
}

void SceneManager::AddEntity(Entity* entity)
{
	entity->OnInitScene();
}

void SceneManager::RemoveEntities()
{
	unsigned int removeSize = static_cast<unsigned int>(m_ToRemove.size()) - 1;
	for (int i = removeSize; i >= 0; --i)
	{
		RemoveEntity(m_ToRemove[i].ent, m_ToRemove[i].scene);
	}
	m_ToRemove.clear();
}

void SceneManager::SetGameOverScene(Scene* scene)
{
	if (scene != nullptr)
	{
		m_pGameOverScene = scene;
	}
	else
	{
		BL_LOG_CRITICAL("SetGameOverScene:: scene was nullptr");
	}
}

void SceneManager::SetScene(Scene* scene)
{
	if (scene == m_pActiveScene)
	{
		BL_LOG_WARNING("SetScene on same scene %s\n", scene->GetName().c_str());
		return;
	}

	ResetScene();

	if (m_pActiveScene != nullptr)
	{
		std::map<std::string, Entity*> oldEntities = *m_pActiveScene->GetEntities();

		for (auto const& [entityName, entity] : oldEntities)
		{
			entity->OnUnInitScene();
		}

		for (auto pair : oldEntities)
		{
			Entity* ent = pair.second;
			if (ent->IsEntityDynamic() == true)
			{
				m_pActiveScene->RemoveEntity(ent->GetName());
			}
		}
	}
	
	// init the active scenes
	std::map<std::string, Entity*> entities = *scene->GetEntities();
	for (auto const& [entityName, entity] : entities)
	{
		entity->SetEntityState(false);
		entity->OnInitScene();
	}

	m_pActiveScene = scene;

	Renderer* renderer = &Renderer::GetInstance();
	renderer->prepareScene(m_pActiveScene);

	if (m_pActiveScene->GetMainCamera() == nullptr)
	{
		m_pActiveScene->SetPrimaryCamera(renderer->m_pScenePrimaryCamera);
	}
	scene->OnInit();
}

void SceneManager::ResetScene()
{
	TODO("Fix this");
	//Renderer::GetInstance().waitForGPU();

	/* ------------------------- GPU -------------------------*/
	
	Renderer::GetInstance().OnResetScene();

	/* ------------------------- GPU -------------------------*/
}

bool SceneManager::sceneExists(std::string sceneName) const
{
    for (auto pair : m_Scenes)
    {
        // A Scene with this m_Name already exists
        if (pair.first == sceneName)
        {
            return true;
        }
    }

    return false;
}
