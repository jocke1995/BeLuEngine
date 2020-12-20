#include "BeLuEngine.h"
#include "Misc/Multithreading/MultiThreadedTask.h"
#include <cstdlib>

Scene* TestScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);
void SponzaUpdateScene(SceneManager* sm, double dt);

class PrintClass : public MultiThreadedTask
{
public:
    PrintClass()
        :MultiThreadedTask(FLAG_THREAD::TEST)
    {

    }
    void Execute()
    {
        int array[10000] = {};
        for (unsigned int i = 0; i < 10000; i++)
        {
            array[i] = rand() % 10 + 1;
        }

        int result = 0;
        for (unsigned int i = 0; i < 10000; i++)
        {
            result += array[i];
        }

        //Log::Print("First Result: %d\n", result);
    }
};

class PrintClass2 : public MultiThreadedTask
{
public:
    PrintClass2()
        :MultiThreadedTask(FLAG_THREAD::TEST2)
    {

    }
    void Execute()
    {
        int array[100] = {};
        for (unsigned int i = 0; i < 100; i++)
        {
            array[i] = rand() % 10 + 1;
        }

        int result = 0;
        for (unsigned int i = 0; i < 100; i++)
        {
            result += array[i];
        }

        //Log::Print("Second Result: %d\n", result);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    /* ------ Engine  ------ */
    BeLuEngine engine;
    engine.Init(hInstance, nCmdShow);

    /*  ------ Get references from engine  ------ */
    Window* const window = engine.GetWindow();
    Timer* const timer = engine.GetTimer();
    ThreadPool* const threadPool = engine.GetThreadPool();
    SceneManager* const sceneManager = engine.GetSceneHandler();
    Renderer* const renderer = engine.GetRenderer();

  //  unsigned int numPrintClasses1 = 600000;
  //  unsigned int numPrintClasses2 = 60000;
  // std::vector<PrintClass*> printClasses;
  // //printClasses.reserve(50000);
  // std::vector<PrintClass2*> printClasses2;
  // //printClasses2.reserve(1000);
  // for (unsigned int i = 0; i < numPrintClasses1; i++)
  // {
  //     printClasses.push_back(new PrintClass());
  // }
  // 
  // for (unsigned int i = 0; i < numPrintClasses2; i++)
  // {
  //     printClasses2.push_back(new PrintClass2());
  // }
  // 
  // timer->StartTimer();
  // for (unsigned int i = 0; i < numPrintClasses1; i++)
  // {
  //     srand(time(NULL));
  //     threadPool->AddTask(printClasses[i]);
  //     //printClasses[i]->Execute();
  // }
  // 
  // for (unsigned int i = 0; i < numPrintClasses2; i++)
  // {
  //     srand(time(NULL));
  //     threadPool->AddTask(printClasses2[i]);
  //     //printClasses[i]->Execute();
  // }
  // 
  // threadPool->WaitForThreads(FLAG_THREAD::TEST);
  // 
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // Log::Print(" -------------------- NO MORE FIRST RESULT -------------------------------\n");
  // 
  // threadPool->WaitForThreads(FLAG_THREAD::TEST2);
  // 
  // double time = timer->StopTimer();
  // 
  // Log::Print("Total time: %f\n", time);
  // 
  // for (unsigned int i = 0; i < numPrintClasses1; i++)
  // {
  //     delete printClasses[i];
  // }
  // 
  // for (unsigned int i = 0; i < numPrintClasses2; i++)
  // {
  //     delete printClasses2[i];
  // }

    /*------ AssetLoader to load models / textures ------*/
   AssetLoader* al = AssetLoader::Get();
   
   //Scene* scene = SponzaScene(sceneManager);
   Scene* scene = TestScene(sceneManager);
   
   
   
   // Set scene
   sceneManager->SetScene(scene);
   
   Log::Print("Entering Game-Loop ...\n\n");
   while (!window->ExitWindow())
   {
       // Temporary functions to test functionalities in the engine
       if (window->WasSpacePressed() == true)
       {
           // Get camera Pos
           component::CameraComponent* cc = scene->GetEntity("player")->GetComponent<component::CameraComponent>();
           DirectX::XMFLOAT3 position = cc->GetCamera()->GetPosition();
           DirectX::XMFLOAT3 lookAt = cc->GetCamera()->GetDirection();
           DirectX::XMFLOAT3 lightPos = position + (lookAt * 20);
   
           static unsigned int counter = 1;
           std::string name = "pointLight" + std::to_string(counter);
           /* ---------------------- PointLightDynamic ---------------------- */
           Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");
           Entity* entity = scene->AddEntity(name);
           component::ModelComponent* mc = entity->AddComponent<component::ModelComponent>();
           component::TransformComponent* tc = entity->AddComponent<component::TransformComponent>();
           component::PointLightComponent* plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);
   
           mc->SetModel(sphereModel);
           mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
           tc->GetTransform()->SetScale(0.3f);
           tc->GetTransform()->SetPosition(lightPos);
   
           plc->SetColor({ 10.0f, 5.0f, 5.0f });
   
           counter++;
   
           sceneManager->AddEntity(entity);
           /* ---------------------- SpotLightDynamic ---------------------- */
       }
       if (window->WasTabPressed() == true)
       {
           // Get camera Pos
           component::CameraComponent* cc = scene->GetEntity("player")->GetComponent<component::CameraComponent>();
           DirectX::XMFLOAT3 position = cc->GetCamera()->GetPosition();
           Log::Print("CameraPos: %f, %f, %f\n", position.x, position.y, position.z);
       }
   
       /* ------ Update ------ */
       timer->Update();
   
       sceneManager->Update(timer->GetDeltaTime());
   
       /* ------ Sort ------ */
       renderer->SortObjects();
   
       /* ------ Draw ------ */
       renderer->Execute();
   }
    
    return EXIT_SUCCESS;
}

Scene* TestScene(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("TestScene");

    component::CameraComponent* cc = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::InputComponent* ic = nullptr;
    component::BoundingBoxComponent* bbc = nullptr;
    component::PointLightComponent* plc = nullptr;
    component::DirectionalLightComponent* dlc = nullptr;
    component::SpotLightComponent* slc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* floorModel = al->LoadModel(L"../Vendor/Resources/Models/FloorPBR/floor.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Floor ---------------------- */
    entity = scene->AddEntity("floor");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(floorModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(50, 1, 50);
    tc->GetTransform()->SetPosition(0.0f, 0.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */

     /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(0, 4, 30);
    /* ---------------------- Sphere ---------------------- */

    /* ---------------------- Poster ---------------------- */
    //entity = scene->AddEntity("poster");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //
    //mc = entity->GetComponent<component::ModelComponent>();
    //mc->SetModel(posterModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    //tc = entity->GetComponent<component::TransformComponent>();
    //tc->GetTransform()->SetScale(2, 1, 2);
    //tc->GetTransform()->SetRotationZ(-PI / 2);
    //tc->GetTransform()->SetPosition(28.5f, 2.0f, 34.0f);
    /* ---------------------- Poster ---------------------- */

    /* ---------------------- SpotLightDynamic ---------------------- */
    //entity = scene->AddEntity("spotLightDynamic");
    //mc = entity->AddComponent<component::ModelComponent>();
    //tc = entity->AddComponent<component::TransformComponent>();
    //slc = entity->AddComponent<component::SpotLightComponent>(FLAG_LIGHT::CAST_SHADOW);
    //
    //float3 pos = { 5.0f, 20.0f, 5.0f };
    //mc->SetModel(sphereModel);
    //mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    //tc->GetTransform()->SetScale(0.3f);
    //tc->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
    //
    ////slc->SetColor({ 0.0f, 1.0f, 0.0f });
    //slc->SetAttenuation({ 1.0, 0.09f, 0.032f });
    //slc->SetPosition(pos);
    //slc->SetDirection({ 0.0f, -1.0f, 0.5f });
    //slc->SetOuterCutOff(50.0f);
    /* ---------------------- SpotLightDynamic ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(FLAG_LIGHT::CAST_SHADOW);
    dlc->SetColor({ 0.6f, 0.6f, 0.6f });
    dlc->SetDirection({ 1.0f, -1.0f, 0.0f });
    dlc->SetCameraTop(30.0f);
    dlc->SetCameraBot(-30.0f);
    dlc->SetCameraLeft(-70.0f);
    dlc->SetCameraRight(70.0f);
    /* ---------------------- dirLight ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&TestUpdateScene);
    return scene;
}

Scene* SponzaScene(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("SponzaScene");

    component::CameraComponent* cc = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::InputComponent* ic = nullptr;
    component::BoundingBoxComponent* bbc = nullptr;
    component::PointLightComponent* plc = nullptr;
    component::DirectionalLightComponent* dlc = nullptr;
    component::SpotLightComponent* slc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Resources/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* sphereModel = al->LoadModel(L"../Vendor/Resources/Models/SpherePBR/ball.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sponza);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetPosition({0.0f, 0.0f, 0.0f});
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Braziers ---------------------- */
    entity = scene->AddEntity("Brazier0");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 0.0f, 15.0f });

    entity = scene->AddEntity("Brazier1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, -42.6f });
    plc->SetColor({ 10.0f, 0.0f, 10.0f });

    entity = scene->AddEntity("Brazier2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 15.0f, 0.0f });

    entity = scene->AddEntity("Brazier3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(FLAG_LIGHT::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, -42.6f });
    plc->SetColor({ 15.0f, 0.0f, 0.0f });
    /* ---------------------- Braziers ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(FLAG_LIGHT::CAST_SHADOW);
    dlc->SetColor({ 0.17, 0.25, 0.3f});
    dlc->SetCameraDistance(300);
    dlc->SetDirection({ -1.0f, -2.0f, 0.03f });
    dlc->SetCameraTop(800.0f);
    dlc->SetCameraBot(-550.0f);
    dlc->SetCameraLeft(-550.0f);
    dlc->SetCameraRight(550.0f);
    dlc->SetCameraFarZ(5000);
    /* ---------------------- dirLight ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&SponzaUpdateScene);
    return scene;
}

void TestUpdateScene(SceneManager* sm, double dt)
{
    //static float intensity = 0.0f;
    //component::SpotLightComponent* slc = sm->GetScene("TestScene")->GetEntity("spotLightDynamic")->GetComponent<component::SpotLightComponent>();
    //float col = abs(sinf(intensity)) * 30;
    //
    //slc->SetColor({ col * 0.2f, 0.0f, col });
    //
    //intensity += 0.5f * dt;
}

void SponzaUpdateScene(SceneManager* sm, double dt)
{

}
