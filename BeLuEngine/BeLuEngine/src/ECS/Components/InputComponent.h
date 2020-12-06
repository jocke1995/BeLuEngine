#ifndef INPUTCOMPONENT_H
#define INPUTCOMPONENT_H
#include "EngineMath.h"
#include "Component.h"
#include "Core.h"

class BaseCamera;
struct MouseScroll;
struct MovementInput;
struct MouseMovement;

namespace component
{
	class InputComponent : public Component
	{
	public:
		// Default Settings
		InputComponent(Entity* parent);
		virtual ~InputComponent();

		virtual void Update(double dt) override;
		void OnInitScene() override;
		void OnUnInitScene() override;

	private:

		// Move camera
		void move(MovementInput* event);
		// Rotate camera
		void rotate(MouseMovement* event);
	};
}

#endif