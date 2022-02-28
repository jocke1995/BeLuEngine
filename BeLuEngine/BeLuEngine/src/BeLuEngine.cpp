#include "stdafx.h"
#include "BeLuEngine.h"
#include "Misc/MultiThreading/Thread.h"

#include "../Misc/EngineStatistics.h"

#include "Renderer/API/Interface/IGraphicsManager.h"

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
	int windowWidth = 1920;
	int windowHeight = 1080;

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
	ThreadPool::GetInstance(numThreads * 2);

	// Sub-engines
	{
		// Renderer
		{
			E_RESOLUTION_TYPES startResolution = E_RESOLUTION_TYPES::Res_1920_1080;	// TODO: This could be saved to options and be read from file next time game starts...

			IGraphicsManager* graphicsManager = IGraphicsManager::Create(E_GRAPHICS_API::D3D12);
			graphicsManager->Init(m_pWindow->GetHwnd(), startResolution, BL_FORMAT_R16G16B16A16_FLOAT);

			m_pRenderer = &Renderer::GetInstance();
			m_pRenderer->Init(startResolution);
		}

		// Physics, sound etc..
	}

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

SceneManager* const BeLuEngine::GetSceneHandler() const
{
	return m_pSceneManager;
}

Renderer* const BeLuEngine::GetRenderer() const
{
	return m_pRenderer;
}
