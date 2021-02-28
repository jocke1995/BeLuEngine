#ifndef SCENEMANAGER_H
#define SCENEMANAGER_H

class Scene;
class Entity;
class Renderer;

// Event
struct SceneChange;

class SceneManager 
{
public:
	static SceneManager& GetInstance();
	~SceneManager();
	
	// Update
	void Update(double dt);

	// Scene
	Scene* CreateScene(std::string sceneName);
	void SetScene(Scene* scene);
	Scene* GetActiveScene();
	Scene* GetScene(std::string sceneName) const;

	void ResetScene();

	// Special scenes
	void SetGameOverScene(Scene* scene);

	// Entity
	void RemoveEntity(Entity* entity, Scene* scene);
	void AddEntity(Entity* entity);

	void RemoveEntities();

private:
	friend class BeLuEngine;
	SceneManager();
	void deleteSceneManager();

	std::map<std::string, Scene*> m_Scenes;
	Scene* m_pActiveScene = nullptr;
	Scene* m_pDefaultScene = nullptr;

	struct EntityScene
	{
		Entity* ent;
		Scene* scene;
	};

	std::vector<EntityScene> m_ToRemove;

	Scene* m_pGameOverScene = nullptr;

	bool sceneExists(std::string sceneName) const;
};

#endif
