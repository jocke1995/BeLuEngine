#include "stdafx.h"
#include "Entity.h"

Entity::Entity(std::string entityName)
{
	m_Id = s_IdCounter++;
	m_Name = entityName;
}

Entity::~Entity()
{
	for (Component* component : m_Components)
	{
		component->OnUnInitScene();
		delete component;
	}
}

bool Entity::operator==(const Entity& other) const
{
	return m_Id == other.m_Id;
}

bool Entity::operator!=(const Entity& other) const
{
	return !(operator==(other));
}

unsigned int Entity::GetID() const
{
	return m_Id;
}

std::string Entity::GetName() const
{
	return m_Name;
}

void Entity::Update(double dt)
{
	for (Component* component : m_Components)
	{
		component->Update(dt);
	}
}

void Entity::OnInitScene()
{
	// for each component in entity: call their implementation of InitScene(),
	// which calls their specific init function (render, audio, game, physics etc)

	for (int i = 0; i < m_Components.size(); i++)
	{
		m_Components.at(i)->OnInitScene();
	}
}

void Entity::OnUnInitScene()
{
	// for each component in entity: call their implementation of InitScene(),
	// which calls their specific init function (render, audio, game, physics etc)
	for (int i = 0; i < m_Components.size(); i++)
	{
		m_Components.at(i)->OnUnInitScene();
	}
}

std::vector<Component*>* Entity::GetAllComponents()
{
	return &m_Components;
}

void Entity::SetEntityState(bool dynamic)
{
	m_Dynamic = dynamic;
}

bool Entity::IsEntityDynamic() const
{
	return m_Dynamic;
}
