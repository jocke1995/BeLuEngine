#include "BeLuEngine.h"

Scene* TestScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);
void SponzaUpdateScene(SceneManager* sm, double dt);

#include "Misc/Multithreading/MultiThreadedTask.h"

class TestAsyncThread : public MultiThreadedTask
{
public:
    TestAsyncThread()
        :MultiThreadedTask(F_THREAD_FLAGS::TEST)
    {};

    void Execute()
    {
        while (true)
        {
            Sleep(200);
            Log::Print("Async!\n");
        }
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

    /*------ AssetLoader to load models / textures ------*/
   AssetLoader* al = AssetLoader::Get();
   
   Scene* scene = SponzaScene(sceneManager);
   //Scene* scene = TestScene(sceneManager);

   // Set scene
   sceneManager->SetScene(scene);
   
   TestAsyncThread test1 = TestAsyncThread();

   //threadPool->AddTask(static_cast<MultiThreadedTask*>(&test1));
   //test1.Execute();

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
           component::PointLightComponent* plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);
           component::BoundingBoxComponent* bbc = entity->AddComponent<component::BoundingBoxComponent>(F_BOUNDING_BOX_FLAGS::PICKING);;
   
           mc->SetModel(sphereModel);
           mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
           tc->GetTransform()->SetScale(0.3f);
           tc->GetTransform()->SetPosition(lightPos);
           bbc->Init();
   
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
       // For easier debugging purposes
       if (SINGLE_THREADED_RENDERER == true)    
           renderer->ExecuteST();
       else
           renderer->ExecuteMT();
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
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Floor ---------------------- */
    entity = scene->AddEntity("floor");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(floorModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(50, 1, 50);
    tc->GetTransform()->SetPosition(0.0f, 0.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */

     /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(0, 4, 30);
    /* ---------------------- Sphere ---------------------- */

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
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sponza);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetPosition({0.0f, 0.0f, 0.0f});
    tc->GetTransform()->SetScale(0.3f, 0.3f, 0.3f);
    /* ---------------------- Sponza ---------------------- */

    /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>(F_BOUNDING_BOX_FLAGS::PICKING);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 15, 5);
    bbc->Init();

    entity = scene->AddEntity("sphere2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>(F_BOUNDING_BOX_FLAGS::PICKING);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 25, 10);
    bbc->Init();

    entity = scene->AddEntity("sphere3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>(F_BOUNDING_BOX_FLAGS::PICKING);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(5, 35, 15);
    bbc->Init();
    /* ---------------------- Sphere ---------------------- */

    /* ---------------------- Braziers ---------------------- */
    entity = scene->AddEntity("Brazier0");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 0.0f, 15.0f });
    
    entity = scene->AddEntity("Brazier1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ -185.0f, 40.0f, -42.6f });
    plc->SetColor({ 10.0f, 0.0f, 10.0f });

    entity = scene->AddEntity("Brazier2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, 66.0f });
    plc->SetColor({ 0.0f, 15.0f, 0.0f });

    entity = scene->AddEntity("Brazier3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition({ 146.0f, 40.0f, -42.6f });
    plc->SetColor({ 15.0f, 0.0f, 0.0f });
    /* ---------------------- Braziers ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(F_LIGHT_FLAGS::CAST_SHADOW);
    dlc->SetColor({ 0.17, 0.25, 0.3f});
    dlc->SetDirection({ -1.0f, -2.0f, 0.03f });
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
