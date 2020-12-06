#include "InputComponent.h"
#include "../Events/EventBus.h"
#include "../Entity.h"
#include "../Renderer/Camera/PerspectiveCamera.h"
#include "../Renderer/Model/Transform.h"

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

	// Check if the key has just been pressed or just been released and convert to a float. Multiply by two and subtract one to get 1 for true and -1 for false. If
	// the key has been pressed, the player should start moving in the direction specified by the key -- hence the value 1. If the key has been released, the player's
	// movement should be negated in the direction specified by the key -- hence the value -1
	int pressed = (static_cast<int>(event->pressed) * 2 - 1);

	// Find out which key has been pressed. Convert to float to get the value 1 if the key pressed should move the player in the positive
	// direction and the value -1 if the key pressed should move the player in the negative direction
	int3 moveCam =
	{
		((event->key == SCAN_CODES::D) - (event->key == SCAN_CODES::A)) * pressed,
		((event->key == SCAN_CODES::R) - (event->key == SCAN_CODES::F)) * pressed,
		((event->key == SCAN_CODES::W) - (event->key == SCAN_CODES::S)) * pressed,
	};

	pc->UpdateMovement(moveCam);
}

void component::InputComponent::rotate(MouseMovement* event)
{
	// Get sibling CameraComponent, expecting the user never to remove this component..
	component::CameraComponent* cc = m_pParent->GetComponent<component::CameraComponent>();
	PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());

	pc->RotateCamera(event->x, event->y);
}
