#ifndef ENTITY_H
#define ENTITY_H

// Renderer
#include "Components/ModelComponent.h"
#include "Components/TransformComponent.h"
#include "Components/CameraComponent.h"
#include "Components/BoundingBoxComponent.h"
#include "Components/InputComponent.h"

// Lights
#include "Components/Lights/PointLightComponent.h"
#include "Components/Lights/DirectionalLightComponent.h"
#include "Components/Lights/SpotLightComponent.h"

class Entity
{
public:
	Entity(std::string entityName);
	virtual ~Entity();

	// Compared using a unique ID
	bool operator == (const Entity& other) const;
	bool operator != (const Entity& other) const;

	template <class T, typename... Args>
	T* AddComponent(Args... args);

	template <class T>
	T* GetComponent() const;

	template <class T>
	bool HasComponent() const;

	unsigned int GetID() const;
	std::string GetName() const;

	void Update(double dt);

	void OnInitScene();
	void OnUnInitScene();

	std::vector<Component*>* GetAllComponents();

	void SetEntityState(bool dynamic);
	bool IsEntityDynamic() const;

private:
	friend class SceneManager;

	unsigned int m_Id = 0;
	inline static unsigned int s_IdCounter = 0;
	std::string m_Name = "";
	
	// Multiple m_pScenes can use the same entity (player for example) and only init/uninit once.
	unsigned int m_ReferenceCount = 0;

	std::vector<Component*> m_Components;

	// All entities will be assumed to be dynamic
	bool m_Dynamic = true;
};

template<class T, typename... Args>
inline T* Entity::AddComponent(Args... args)
{
	// Check if component already exists,
	// and if it does.. return it
	T* compInEntity = GetComponent<T>();
	if (compInEntity == nullptr)
	{
		// Add component
		T* finalComponent = new T(this, std::forward<Args>(args)...);
		m_Components.push_back(finalComponent);

		return finalComponent;
	}
	return compInEntity;
}

template<class T>
inline T* Entity::GetComponent() const
{
	for (int i = 0; i < m_Components.size(); i++)
	{
		T* component = dynamic_cast<T*>(m_Components[i]);

		if (component != nullptr)
		{
			return component;
		}
	}
	return nullptr;
}

template<class T>
inline bool Entity::HasComponent() const
{
	for (int i = 0; i < m_Components.size(); i++)
	{
		T* component = dynamic_cast<T*>(m_Components[i]);

		if (component != nullptr)
		{
			// Found
			return true;
		}
	}
	return false;
}

#endif
