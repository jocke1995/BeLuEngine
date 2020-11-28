#include "InputComponent.h"
#include "../Events/EventBus.h"
#include "../Entity.h"
#include "../Renderer/Camera/PerspectiveCamera.h"
#include "../Renderer/Transform.h"

#include "Core.h"
#include "EngineMath.h"

component::InputComponent::InputComponent(Entity* parent)
	:Component(parent)
{
}

component::InputComponent::~InputComponent()
{
}

void component::InputComponent::Update(double dt)
{
	
}

void component::InputComponent::OnInitScene()
{
	EventBus::GetInstance().Subscribe(this, &InputComponent::move);
	EventBus::GetInstance().Subscribe(this, &InputComponent::rotate);
}

void component::InputComponent::OnUnInitScene()
{
	EventBus::GetInstance().Unsubscribe(this, &InputComponent::move);
	EventBus::GetInstance().Unsubscribe(this, &InputComponent::rotate);
}

void component::InputComponent::move(MovementInput* event)
{
	// Get sibling CameraComponent, expecting the user never to remove this component..
	component::CameraComponent* cc = m_pParent->GetComponent<component::CameraComponent>();
	PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());

	// Find out which key has been pressed. Convert to float to get the value 1 if the key pressed should move the player in the positive
	// direction and the value -1 if the key pressed should move the player in the negative direction
	float3 moveCam =
	{
		(static_cast<float>(event->key == SCAN_CODES::D) - static_cast<float>(event->key == SCAN_CODES::A)) * event->pressed,
		(static_cast<float>(event->key == SCAN_CODES::R) - static_cast<float>(event->key == SCAN_CODES::F)) * event->pressed,
		(static_cast<float>(event->key == SCAN_CODES::W) - static_cast<float>(event->key == SCAN_CODES::S)) * event->pressed,
	};

	pc->MoveCamera(moveCam);
}

void component::InputComponent::rotate(MouseMovement* event)
{
	// Get sibling CameraComponent, expecting the user never to remove this component..
	component::CameraComponent* cc = m_pParent->GetComponent<component::CameraComponent>();
	PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());

	pc->RotateCamera(event->x, event->y);
}
