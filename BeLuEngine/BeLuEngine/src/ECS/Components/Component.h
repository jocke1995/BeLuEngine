#ifndef COMPONENT_H
#define COMPONENT_H

class Entity;
class Renderer;

class Component
{
public:
	Component(Entity* parent);
	virtual ~Component();

	virtual void Update(double dt);

	//SceneInit
	virtual void OnInitScene() = 0;
	virtual void OnUnInitScene() = 0;

	virtual void Reset();

	Entity* const GetParent() const;

protected:
	Entity* m_pParent = nullptr;
};

#endif
