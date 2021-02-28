#include "stdafx.h"
#include "Component.h"
#include "../Renderer/Renderer.h"
#include "../BeLuEngine.h"

Component::Component(Entity* parent)
{
	m_pParent = parent;
}

Component::~Component()
{
}

void Component::Update(double dt)
{
}

void Component::Reset()
{
}

Entity* const Component::GetParent() const
{
	return m_pParent;
}
