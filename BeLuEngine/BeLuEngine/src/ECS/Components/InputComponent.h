#ifndef INPUTCOMPONENT_H
#define INPUTCOMPONENT_H
#include "EngineMath.h"
#include "Component.h"
#include "Core.h"

class BaseCamera;
class MouseScroll;
class MovementInput;
class MouseMovement;

namespace component
{
	class InputComponent : public Component
	{
	public:
		// Default Settings
		InputComponent(Entity* parent);
		virtual ~InputComponent();

		virtual void Update(double dt);

		void OnInitScene();
		void OnUnInitScene();
	private:

		// Move camera
		void move(MovementInput* event);
		// Rotate camera
		void rotate(MouseMovement* event);
	};
}

#endif