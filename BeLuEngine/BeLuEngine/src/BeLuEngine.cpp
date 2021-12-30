#include "stdafx.h"
#include "BeLuEngine.h"
#include "Misc/MultiThreading/Thread.h"

#include "../Misc/EngineStatistics.h"

#include "Renderer/API/IGraphicsManager.h"

BeLuEngine::BeLuEngine()
{
	
}

BeLuEngine::~BeLuEngine()
{
	BL_SAFE_DELETE(m_pTimer);

	m_pSceneManager->deleteSceneManager();
	m_pRenderer->deleteRenderer();

	BL_SAFE_DELETE(m_pWindow);

	BL_ASSERT(AssetLoader::Get()->DeleteAllAssets());

	IGraphicsManager* graphicsManager = IGraphicsManager::GetBaseInstance();
	graphicsManager->Destroy();
}

void BeLuEngine::Init(HINSTANCE hInstance, int nCmdShow)
{
	// Window values
	bool windowedFullscreen = false;
	int windowWidth = 1280;
	int windowHeight = 720;

	EngineStatistics::GetIM_CommonStats().m_ResX = windowWidth;
	EngineStatistics::GetIM_CommonStats().m_ResY = windowHeight;

	// Misc
	Window::Create(hInstance, nCmdShow, windowedFullscreen, windowWidth, windowHeight);
	m_pWindow = Window::GetInstance();
	m_pTimer = new Timer(m_pWindow);

	// ThreadPool
	int numThreads = std::thread::hardware_concurrency();
	if (numThreads == 0) // function not supported
	{
		BL_LOG_WARNING("Only 1 core on CPU, might be very laggy!\n");
		numThreads = 1;
	}
	
	EngineStatistics::GetIM_CommonStats().m_NumCpuCores = numThreads;
	m_pThreadPool = &ThreadPool::GetInstance(numThreads * 2);

	// Sub-engines
	IGraphicsManager* graphicsManager = IGraphicsManager::Create(E_GRAPHICS_API::D3D12);
	graphicsManager->Init(m_pWindow->GetHwnd(), windowWidth, windowHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);

	m_pRenderer = &Renderer::GetInstance();
	m_pRenderer->InitD3D12(m_pWindow->GetHwnd(), windowWidth, windowHeight, hInstance, m_pThreadPool);

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
