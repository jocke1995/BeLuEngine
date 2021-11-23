#include "BeLuEngine.h"

Scene* TestScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void TestUpdateScene(SceneManager* sm, double dt);
void SponzaUpdateScene(SceneManager* sm, double dt);

// Temp
#include "Misc/Multithreading/MultiThreadedTask.h"

class TestAsyncThread : public MultiThreadedTask
{
public:
    TestAsyncThread()
        :MultiThreadedTask(F_THREAD_FLAGS::TEST)
    {};

    void Execute()
    {
        //while (true)
        //{
            Sleep(200);
            //Log::Print("Async!\n");
        //}
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

   Log::Print("Entering Game-Loop ...\n\n");
   while (!window->ExitWindow())
   {
       // Async Test
       //threadPool->AddTask(static_cast<MultiThreadedTask*>(&test1));

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
   
           plc->SetColor({ 0.5f, 0.25f, 0.25f });
           entity->Update(0);

           counter++;
   
           sceneManager->AddEntity(entity);

           Log::Print("X: %f, Y:%f, Z: %f\n", position.x, position.y, position.z);
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
    Model* posterModel = al->LoadModel(L"../Vendor/Resources/Models/Poster/Poster.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Mirror ---------------------- */
    entity = scene->AddEntity("Mirror");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(posterModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(30.0f);
    tc->GetTransform()->SetRotationX(PI / 2);
    tc->GetTransform()->SetRotationY(PI);
    tc->GetTransform()->SetPosition(0, 30, 40);

    MaterialData* sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    /* ---------------------- Mirror ---------------------- */

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
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(0, 4, 0);

    bbc->Init();
    /* ---------------------- Sphere ---------------------- */

    /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE);

    sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;
    tc->GetTransform()->SetScale(2.0f);
    tc->GetTransform()->SetPosition(-5.0f, 11, 30);

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    mc->Update(0);

    entity = scene->AddEntity("sphere2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);

    sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(15, 4, 4);

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    mc->Update(0);


    entity = scene->AddEntity("sphere3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);

    sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(15, 7, 7);

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    mc->Update(0);
    /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("dirLight1");

    dlc = entity->AddComponent<component::DirectionalLightComponent>();
    dlc->SetColor({ 0.1f, 0.25f, 0.3f });
    dlc->SetDirection({ -1.0f, -2.0f, 0.03f });
    dlc->SetIntensity(5.0f);

    dlc->Update(0);
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
    Model* boxModel = al->LoadModel(L"../Vendor/Resources/Models/CubePBR/cube.obj");
    Model* posterModel = al->LoadModel(L"../Vendor/Resources/Models/Poster/Poster.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Mirror1 ---------------------- */
    entity = scene->AddEntity("Mirror1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(posterModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(6.0f);
    //tc->GetTransform()->SetRotationX(PI/2 + 0.5);
    //tc->GetTransform()->SetRotationY(PI / 4);
    tc->GetTransform()->SetRotationZ(PI / 2);
    tc->GetTransform()->SetPosition(55, 4.5, 0);

    MaterialData* sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    /* ---------------------- Mirror1 ---------------------- */

    /* ---------------------- Mirror2 ---------------------- */
    entity = scene->AddEntity("Mirror2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(posterModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(6.0f);
    //tc->GetTransform()->SetRotationX(PI/2 + 0.5);
    tc->GetTransform()->SetRotationY(PI);
    tc->GetTransform()->SetRotationZ(PI / 2);
    tc->GetTransform()->SetPosition(-55, 4.5, 0);

    sharedMatData = mc->GetMaterialAt(0)->GetSharedMaterialData();
    sharedMatData->hasMetallicTexture = false;
    sharedMatData->hasRoughnessTexture = false;
    sharedMatData->hasNormalTexture = false;
    sharedMatData->metallicValue = 0.99f;
    sharedMatData->roughnessValue = 0.10f;

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    /* ---------------------- Mirror2 ---------------------- */

    /* ---------------------- Sponza ---------------------- */
    entity = scene->AddEntity("sponza");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    
    mc->SetModel(sponza);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetPosition({0.0f, 0.0f, 0.0f});
    tc->GetTransform()->SetScale(0.05f, 0.05f, 0.05f);
    /* ---------------------- Sponza ---------------------- */

    


    entity = scene->AddEntity("box");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(boxModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetPosition(0, 4.0f, 1.0f);
    bbc->Init();


    /* ---------------------- Sphere ---------------------- */
    entity = scene->AddEntity("sphere1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();
    
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_TRANSPARENT | F_DRAW_FLAGS::NO_DEPTH );
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(15, 1, 1);
    bbc->Init();

    entity = scene->AddEntity("sphere2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    //mc->SetMaterialAt(0, ballMatCopy);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(15, 4, 4);
    bbc->Init();

    entity = scene->AddEntity("sphere3");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent<component::BoundingBoxComponent>();

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(15, 7, 7);
    bbc->Init();
    /* ---------------------- Sphere ---------------------- */

    /* ---------------------- Lights ---------------------- */
    entity = scene->AddEntity("pl1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.5f);
    tc->GetTransform()->SetPosition({ 25.0f, 16.2f, 7.5f});
    plc->SetColor({ 1.0f, 1.0f, 1.0f });
    plc->SetIntensity(1.0f);
    entity->Update(0);

    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(F_LIGHT_FLAGS::CAST_SHADOW);
    dlc->SetColor({ 0.1f, 0.25f, 0.3f });
    dlc->SetDirection({ -1.0f, -2.0f, 0.03f });
    /* ---------------------- Lights ---------------------- */

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
