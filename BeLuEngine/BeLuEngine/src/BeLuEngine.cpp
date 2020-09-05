#include "stdafx.h"
#include "BeLuEngine.h"
#include "Misc/Thread.h"

BeLuEngine::BeLuEngine()
{
	
}

BeLuEngine::~BeLuEngine()
{
	delete m_Window;
	delete m_Timer;

	m_ThreadPool->WaitForThreads(FLAG_THREAD::ALL);
	m_ThreadPool->ExitThreads();
	delete m_ThreadPool;

	delete m_SceneManager;
	delete m_Renderer;
}

void BeLuEngine::Init(HINSTANCE hInstance, int nCmdShow)
{
	// Misc
	m_Window = new Window(hInstance, nCmdShow, false);
	m_Timer = new Timer(m_Window);

	// ThreadPool
	int numCores = std::thread::hardware_concurrency();
	if (numCores == 0) numCores = 1; // function not supported
	m_ThreadPool = new ThreadPool(numCores); // Set num m_Threads to number of cores of the cpu

	// Sub-engines
	m_Renderer = new Renderer();
	m_Renderer->InitD3D12(m_Window->GetHwnd(), hInstance, m_ThreadPool);

	// ECS
	m_SceneManager = new SceneManager(m_Renderer);
}

Window* const BeLuEngine::GetWindow() const
{
	return m_Window;
}

Timer* const BeLuEngine::GetTimer() const
{
	return m_Timer;
}

ThreadPool* const BeLuEngine::GetThreadPool() const
{
	return m_ThreadPool;
}

SceneManager* const BeLuEngine::GetSceneHandler() const
{
	return m_SceneManager;
}

Renderer* const BeLuEngine::GetRenderer() const
{
	return m_Renderer;
}
