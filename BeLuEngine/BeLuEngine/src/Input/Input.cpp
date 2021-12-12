#include "stdafx.h"
#include "Input.h"

// Event
#include "..\Events\EventBus.h"

// ImGui
#include "../ImGUI/imgui.h"
#include "../ImGUI/imgui_impl_win32.h"
#include "../ImGUI/imgui_impl_dx12.h"

Input& Input::GetInstance()
{
	static Input instance;

	return instance;
}

void Input::RegisterDevices(HWND hWnd)
{
	static RAWINPUTDEVICE m_Rid[2];

	// Register mouse
	m_Rid[0].usUsagePage = 0x01;
	m_Rid[0].usUsage = 0x02;
	m_Rid[0].dwFlags = 0;
	m_Rid[0].hwndTarget = hWnd;

	// Register keyboard
	m_Rid[1].usUsagePage = 0x01;
	m_Rid[1].usUsage = 0x06;
	m_Rid[1].dwFlags = 0;
	m_Rid[1].hwndTarget = hWnd;

	if (!RegisterRawInputDevices(m_Rid, 2, sizeof(m_Rid[0])))
	{
		BL_LOG_CRITICAL("Device registration error: %f\n", GetLastError());
	}
}

void Input::SetKeyState(SCAN_CODES key, bool pressed)
{
	// If this is true, the key is not currently being pressed. Meaning that its the first time we start to move the camera
	// If this is false, the keyState is currently true. Which means that it is already being pressed, and an event should not be published.
	bool justPressed = !m_KeyState[key];

	m_KeyState[key] = pressed;
	if (key == SCAN_CODES::W ||
		key == SCAN_CODES::A ||
		key == SCAN_CODES::S ||
		key == SCAN_CODES::D ||
		key == SCAN_CODES::R ||
		key == SCAN_CODES::F)
	{
		// Move Camera (when pressing buttons)
		if (justPressed == true)
		{
			EventBus::GetInstance().Publish(&MovementInput(key, true));
		}
		// Stop moving (when releasing buttons)
		else if (pressed == false)
		{
			EventBus::GetInstance().Publish(&MovementInput(key, false));
		}
	}
#ifdef DEBUG
	else if (key == SCAN_CODES::ALT)
	{
		static bool enabled = true;

		if (justPressed == true)
		{
			// Toggle mouse lookaround
			enabled = !enabled;
			EventBus::GetInstance().Publish(&ToggleCameraLookAround(key, enabled));

			ShowCursor(!enabled);

			ImGuiIO& io = ImGui::GetIO();

			if (enabled == false)
			{
				io.ConfigFlags = 0;
			}
			else
			{
				io.ConfigFlags = ImGuiConfigFlags_NoMouse;
			}
		}
	}
#endif
}

void Input::SetMouseButtonState(MOUSE_BUTTON button, bool pressed)
{
	bool alreadyPressed = m_MouseButtonState[button];

	m_MouseButtonState[button] = pressed;

	switch (pressed)
	{
	case true:	// Pressed
		if (alreadyPressed == false)
		{
			EventBus::GetInstance().Publish(&MouseClick(button, pressed));
		}
		break;

	case false:	// Released
		EventBus::GetInstance().Publish(&MouseRelease(button, pressed));
		break;
	}
}

void Input::SetMouseScroll(short scrollAmount)
{
	int mouseScroll = static_cast<int>(scrollAmount > 0) * 2 - 1;
	EventBus::GetInstance().Publish(&MouseScroll(mouseScroll));
}

void Input::SetMouseMovement(int x, int y)
{
	EventBus::GetInstance().Publish(&MouseMovement(x, y));
}

bool Input::GetKeyState(SCAN_CODES key)
{
	return m_KeyState[key];
}

bool Input::GetMouseButtonState(MOUSE_BUTTON button)
{
	return m_MouseButtonState[button];
}

Input::Input()
{
}
