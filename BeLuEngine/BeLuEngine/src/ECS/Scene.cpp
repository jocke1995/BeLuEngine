#include "stdafx.h"
#include "Scene.h"
#include "Entity.h"

#include "../Renderer/Camera/BaseCamera.h"

void DefaultUpdateScene(SceneManager* sm, double dt)
{

}

void DefaultInitScene(Scene* scene)
{
}

Scene::Scene(std::string sceneName)
{
    m_SceneName = sceneName;

    m_pUpdateScene = &DefaultUpdateScene;
    m_pOnInit = &DefaultInitScene;
}

Scene::~Scene()
{
    for (auto pair : m_EntitiesToKeep)
    {
        if (pair.second != nullptr)
        {
            delete pair.second;
        }
    }
}

bool Scene::operator==(const Scene& other)
{
    return m_SceneName == other.m_SceneName;
}

bool Scene::operator!=(const Scene& other)
{
    return !(operator==(other));
}

Entity* Scene::AddEntityFromOther(Entity* other)
{
    if (EntityExists(other->GetName()) == true)
    {
        Log::PrintSeverity(Log::Severity::CRITICAL, "AddEntityFromOther: Trying to add two components with the same name \'%s\' into scene: %s\n", other->GetName().c_str(), m_SceneName.c_str());
        return nullptr;
    }

    m_EntitiesToKeep[other->GetName()] = other;

    m_NrOfEntities++;
    return other;
}

// Returns false if the entity couldn't be created
Entity* Scene::AddEntity(std::string entityName)
{
    if (EntityExists(entityName) == true)
    {
        Log::PrintSeverity(Log::Severity::CRITICAL, "Trying to add two components with the same name \'%s\' into scene: %s\n", entityName.c_str(), m_SceneName.c_str());
        return nullptr;
    }

    m_EntitiesToKeep[entityName] = new Entity(entityName);
    m_NrOfEntities++;

    return m_EntitiesToKeep[entityName];
}

bool Scene::RemoveEntity(std::string entityName)
{
    if (!EntityExists(entityName))
    {
        Log::PrintSeverity(Log::Severity::CRITICAL, "Trying to remove entity \'%s\' that does not exist in scene: %s\n", entityName.c_str(), m_SceneName.c_str());
        return false;
    }

    Entity* ent = m_EntitiesToKeep[entityName];

    m_EntitiesToKeep.erase(entityName);
    delete ent;

    m_NrOfEntities--;
    return true;
}

void Scene::SetPrimaryCamera(BaseCamera* primaryCamera)
{
    m_pPrimaryCamera = primaryCamera;
}

Entity* Scene::GetEntity(std::string entityName)
{
    if (EntityExists(entityName))
    {
        return m_EntitiesToKeep.at(entityName);
    }

    Log::PrintSeverity(Log::Severity::WARNING, "No Entity with name: \'%s\' was found.\n", entityName.c_str());
    return nullptr;
}

const std::map<std::string, Entity*>* Scene::GetEntities() const
{
    return &m_EntitiesToKeep;
}

unsigned int Scene::GetNrOfEntites() const
{
    return m_NrOfEntities;
}

BaseCamera* Scene::GetMainCamera() const
{
    return m_pPrimaryCamera;
}

std::string Scene::GetName() const
{
    return m_SceneName;
}

void Scene::SetUpdateScene(void(*UpdateScene)(SceneManager*, double dt))
{
    m_pUpdateScene = UpdateScene;
}

void Scene::OnInit()
{
    m_pOnInit(this);
}

void Scene::SetOnInit(void(*OnInit)(Scene*))
{
    m_pOnInit = OnInit;
}

void Scene::Update(SceneManager* sm, double dt)
{
    // Run the scenes specific update function
    m_pUpdateScene(sm, dt);

    for (auto pair : m_EntitiesToKeep)
    {
        pair.second->Update(dt);
    }
}

bool Scene::EntityExists(std::string entityName) const
{
    for (auto pair : m_EntitiesToKeep)
    {
        // An entity with this m_Name already exists
        if (pair.first == entityName)
        {
            return true;
        }
    }

    return false;
}
