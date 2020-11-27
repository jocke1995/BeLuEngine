#include "InputComponent.h"
#include "../Events/EventBus.h"
#include "../Entity.h"
#include "../Renderer/Camera/PerspectiveCamera.h"
#include "../Renderer/Transform.h"

#include "Core.h"

component::InputComponent::InputComponent(Entity* parent)
	:Component(parent)
{
}

component::InputComponent::~InputComponent()
{
}

void component::InputComponent::Update(double dt)
{
	// Get sibling CameraComponent, expecting the user never to remove this component..
	component::CameraComponent* cc = m_pParent->GetComponent<component::CameraComponent>();
	PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());

	// yaw/pitch/roll conversion to quaternion
	// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	// Abbreviations for the various angular functions

	float yaw   = pc->GetYaw();
	float pitch = pc->GetPitch();

	double cy = cos(0 * 0.5);
	double sy = sin(0 * 0.5);
	double cp = cos(-yaw * 0.5);
	double sp = sin(-yaw * 0.5);
	double cr = cos(pitch * 0.5);
	double sr = sin(pitch * 0.5);

	DirectX::XMVECTOR rotQuaternion = {
		sr * cp * cy - cr * sp * sy,
		cr * sp * cy + sr * cp * sy,
		cr * cp * sy - sr * sp * cy,
		cr * cp * cy + sr * sp * sy
	};

	DirectX::XMVECTOR rotatedDir;
	// Rotate around quaternion axis
	rotatedDir = DirectX::XMVector3Rotate({ 0, 0, 1 }, rotQuaternion);

	DirectX::XMFLOAT3 newDir;
	DirectX::XMStoreFloat3(&newDir, rotatedDir);


	
	pc->SetDirection(newDir.x, newDir.y, newDir.z);
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

	pc->UpdateMovement(moveCam.x, moveCam.y, moveCam.z);
}

void component::InputComponent::rotate(MouseMovement* event)
{
	// Get sibling CameraComponent, expecting the user never to remove this component..
	component::CameraComponent* cc = m_pParent->GetComponent<component::CameraComponent>();
	PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());

	// Mouse movement
	int x = event->x, y = event->y;

	// rotateX/Y determines how much yaw/pitch changes
	static const float constScale = 1 / 1300.0;

	float rotateX = (static_cast<float>(x) * 2 * constScale);
	float rotateY = -(static_cast<float>(y) * 2 * constScale);

	// Update Yaw and Pitch
	pc->SetYaw(pc->GetYaw()- rotateX);
	pc->SetPitch(pc->GetPitch() - rotateY);

	// Constraints so that you can not look directly under/above the character
	static const float yaw_constraint_offset = 0.01f;

	static float upper = (PI - yaw_constraint_offset) / 2;
	static float lower = -(PI - yaw_constraint_offset) / 2;
	// Clamp between upper and lower
	pc->SetPitch(Max(Min(pc->GetPitch(), upper), lower));
}
