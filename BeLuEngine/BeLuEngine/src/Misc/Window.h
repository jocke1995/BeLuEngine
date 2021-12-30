#ifndef WINDOW_H
#define WINDOW_H
#include <Windows.h>
#include <string>

// Temp
static bool spacePressed = false;
static bool tabPressed = false;

class Window
{
public:
	static void Create(
		HINSTANCE hInstance,
		int nCmdShow,
		bool windowedFullScreen = false,
		int screenWidth = 800,
		int screenHeight = 600,
		LPCTSTR windowName = L"windowName",
		LPCTSTR windowTitle = L"windowTitle");
	~Window();

	static Window* GetInstance();

	void SetWindowTitle(std::wstring newTitle);

	bool IsFullScreen() const;
	int GetScreenWidth() const;
	int GetScreenHeight() const;
	HWND GetHwnd() const;

	void SetScreenWidth(int width);
	void SetScreenHeight(int height);

	bool ExitWindow();

	// Temp
	bool WasSpacePressed();
	bool WasTabPressed();

private:
	Window(
		HINSTANCE hInstance,
		int nCmdShow,
		bool windowedFullScreen = false,
		int screenWidth = 800,
		int screenHeight = 600,
		LPCTSTR windowName = L"windowName",
		LPCTSTR windowTitle = L"windowTitle");

	int m_ScreenWidth;
	int m_ScreenHeight;
	bool m_WindowedFullScreen;
	LPCTSTR m_WindowName;
	LPCTSTR m_WindowTitle;

	HWND m_Hwnd = nullptr;
	bool m_ShutDown;

	bool initWindow(HINSTANCE hInstance, int nCmdShow);
};

#endif