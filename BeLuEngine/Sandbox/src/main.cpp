#include "BeLuEngine.h"

#include "Renderer/Camera/PerspectiveCamera.h"

Scene* TestScene(SceneManager* sm);
void TestUpdateScene(SceneManager* sm, double dt);

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

    Scene* testScene = TestScene(sceneManager);

    // Set scene
    sceneManager->SetScene(testScene);

    while (!window->ExitWindow())
    {
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
    Model* cubeModel = al->LoadModel(L"../Vendor/Resources/Models/CubePBR/cube.obj");
    Model* posterModel = al->LoadModel(L"../Vendor/Resources/Models/Poster/Poster.obj");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    cc = entity->AddComponent<component::CameraComponent>(CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());

    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(1.0f);
    tc->GetTransform()->SetPosition(0, 4, 30);
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

    /* ---------------------- Poster ---------------------- */
    entity = scene->AddEntity("poster");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(posterModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(2, 1, 2);
    tc->GetTransform()->SetRotationZ(-PI / 2);
    tc->GetTransform()->SetPosition(28.5f, 2.0f, 34.0f);
    /* ---------------------- Poster ---------------------- */

    /* ---------------------- SpotLightDynamic ---------------------- */
    entity = scene->AddEntity("spotLightDynamic");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    slc = entity->AddComponent<component::SpotLightComponent>();
    
    float3 pos = { 5.0f, 20.0f, 5.0f };
    mc->SetModel(sphereModel);
    mc->SetDrawFlag(FLAG_DRAW::DRAW_OPAQUE | FLAG_DRAW::GIVE_SHADOW);
    tc->GetTransform()->SetScale(0.3f);
    tc->GetTransform()->SetPosition(pos.x, pos.y, pos.z);
    
    slc->SetColor({ 5.0f, 0.0f, 0.0f });
    slc->SetAttenuation({ 1.0, 0.09f, 0.032f });
    slc->SetPosition(pos);
    slc->SetDirection({ 1.0f, -1.0f, 1.0f });
    slc->SetOuterCutOff(50.0f);
    /* ---------------------- SpotLightDynamic ---------------------- */

    /* ---------------------- dirLight ---------------------- */
    entity = scene->AddEntity("dirLight");
    dlc = entity->AddComponent<component::DirectionalLightComponent>();
    dlc->SetColor({ 0.8f, 0.8f, 0.8f });
    dlc->SetDirection({ -2.0f, -1.0f, -1.0f });
    //dlc->SetCameraTop(30.0f);
    //dlc->SetCameraBot(-30.0f);
    //dlc->SetCameraLeft(-70.0f);
    //dlc->SetCameraRight(70.0f);
    /* ---------------------- dirLight ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&TestUpdateScene);
    return scene;
}

void TestUpdateScene(SceneManager* sm, double dt)
{
    static float intensity = 0.0f;
    component::SpotLightComponent* slc = sm->GetScene("TestScene")->GetEntity("spotLightDynamic")->GetComponent<component::SpotLightComponent>();
    float col = abs(sinf(intensity)) * 20;
    slc->SetColor({ col / 2, 0, col / 3 });
    
    intensity += 0.005f;

    //component::CameraComponent* cc = sm->GetScene("TestScene")->GetEntity("player")->GetComponent<component::CameraComponent>();
    //PerspectiveCamera* pc = static_cast<PerspectiveCamera*>(cc->GetCamera());
}
