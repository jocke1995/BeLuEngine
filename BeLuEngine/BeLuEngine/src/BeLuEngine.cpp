#include "stdafx.h"
#include "BeLuEngine.h"
#include "Misc/MultiThreading/Thread.h"

BeLuEngine::BeLuEngine()
{
	
}

BeLuEngine::~BeLuEngine()
{
	// Gpu will crash if we delete stuff while commandQueues are running
	m_pRenderer->waitForGPU();

	delete m_pWindow;
	delete m_pTimer;

	m_pSceneManager->deleteSceneManager();
	m_pRenderer->deleteRenderer();
}

void BeLuEngine::Init(HINSTANCE hInstance, int nCmdShow)
{
	// Window values
	bool windowedFullscreen = false;
	int windowWidth = 800;
	int windowHeight = 600;

	// Misc
	m_pWindow = new Window(hInstance, nCmdShow, windowedFullscreen, windowWidth, windowHeight);
	m_pTimer = new Timer(m_pWindow);

	// ThreadPool
	int numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) // function not supported
	{
		Log::PrintSeverity(Log::Severity::WARNING, "Only 1 core on CPU, might be very laggy!\n");
		numThreads = 1;
	}

	m_pThreadPool = &ThreadPool::GetInstance(numThreads);

	// Sub-engines
	m_pRenderer = &Renderer::GetInstance();
	m_pRenderer->InitD3D12(m_pWindow, hInstance, m_pThreadPool);

	// ECS
	m_pSceneManager = &SceneManager::GetInstance();

	Input::GetInstance().RegisterDevices(m_pWindow->GetHwnd());
}

Window* const BeLuEngine::GetWindow() const
{
	return m_pWindow;
}

Timer* const BeLuEngine::GetTimer() const
{
	return m_pTimer;
}

ThreadPool* const BeLuEngine::GetThreadPool() const
{
	return m_pThreadPool;
}

SceneManager* const BeLuEngine::GetSceneHandler() const
{
	return m_pSceneManager;
}

Renderer* const BeLuEngine::GetRenderer() const
{
	return m_pRenderer;
}
