#pragma once
#include "../Input/Input.h"
#include "../ECS/Scene.h"

class Entity;
class Event
{
public:
	virtual ~Event() = default;
};

struct MovementInput : public Event
{
	MovementInput(SCAN_CODES key, bool pressed) : key{ key }, pressed{ pressed } {};
	SCAN_CODES key;
	bool pressed;
};

struct MouseMovement : public Event
{
	MouseMovement(int x, int y) : x{ x }, y{ y } {};
	int x, y;
};

struct MouseClick : public Event
{
	MouseClick(MOUSE_BUTTON button, bool pressed) : button{ button }, pressed{ pressed } {};
	MOUSE_BUTTON button;
	bool pressed;
};

struct MouseRelease : public Event
{
	MouseRelease(MOUSE_BUTTON button, bool pressed) : button{ button }, pressed{ pressed } {};
	MOUSE_BUTTON button;
	bool pressed;
};

struct MouseScroll : public Event
{
	MouseScroll(int scroll) : scroll{ scroll } {};
	int scroll;
};

struct ModifierInput : public Event
{
	ModifierInput(SCAN_CODES key, bool pressed) : key{ key }, pressed{ pressed } {};
	SCAN_CODES key;
	bool pressed;
};

struct WindowChange : public Event
{
	WindowChange() {};
};

struct ButtonPressed : public Event
{
	ButtonPressed(std::string name) : name{ name } {};
	std::string name;
};
