#include "BeLuEngine.h"

// Helps intellisense to understand that stdafx is included
//#include "Headers/stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    /* ------ Engine  ------ */
    BeLuEngine beluEngine = BeLuEngine();
    beluEngine.Init(hInstance, nCmdShow);

	/*  ------ Get references from engine  ------ */
	Window* const window = beluEngine.GetWindow();
	Timer* const timer = beluEngine.GetTimer();
	ThreadPool* const threadPool = beluEngine.GetThreadPool();
	SceneManager* const sceneManager = beluEngine.GetSceneHandler();
	Renderer* const renderer = beluEngine.GetRenderer();

    /*------ AssetLoader to load models / textures ------*/
    AssetLoader* al = AssetLoader::Get();

    if (renderer->GetActiveScene())
    {
        while (!window->ExitWindow())
        {
            // Currently no scene set, hence the m_pRenderer should not be working.

            /* ------ Update ------ */
            timer->Update();
            renderer->Update(timer->GetDeltaTime());

            /* ------ Sort ------ */
            renderer->SortObjectsByDistance();

            /* ------ Draw ------ */
            renderer->Execute();
        }
    }

    return 0;
}
