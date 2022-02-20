#include "BeLuEngine.h"

Scene* PBRScene(SceneManager* sm);
Scene* SponzaScene(SceneManager* sm);

void PBRUpdateScene(SceneManager* sm, double dt);
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
   
   //Scene* scene = SponzaScene(sceneManager);
   Scene* scene = PBRScene(sceneManager);

   // Set scene
   sceneManager->SetScene(scene);
   
   TestAsyncThread test1 = TestAsyncThread();

   BL_LOG_INFO("Entering Game-Loop ...\n\n");
   while (!window->ExitWindow())
   {
       // Hacky Async Test
       //threadPool->AddTask(static_cast<MultiThreadedTask*>(&test1));

       // Temporary functions to test functionalities in the renderer
       if (window->WasSpacePressed() == true)
       {
           // Get camera Pos
           component::CameraComponent* cc = scene->GetEntity("player")->GetComponent<component::CameraComponent>();
           DirectX::XMFLOAT3 position = cc->GetCamera()->GetPosition();
           DirectX::XMFLOAT3 lookAt = cc->GetCamera()->GetDirection();
           DirectX::XMFLOAT3 lightPos = position + (lookAt * 10);
   
           static unsigned int counter = 1;
           std::string name = "pointLight" + std::to_string(counter);
           /* ---------------------- PointLightDynamic ---------------------- */
           Model* steelSphere = al->LoadModel(L"../Vendor/Assets/Models/SteelSphere/sphere.obj");
           Entity* entity = scene->AddEntity(name);
           component::ModelComponent* mc = entity->AddComponent<component::ModelComponent>();
           component::TransformComponent* tc = entity->AddComponent<component::TransformComponent>();
           component::PointLightComponent* plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);
           component::BoundingBoxComponent* bbc = entity->AddComponent<component::BoundingBoxComponent>(F_BOUNDING_BOX_FLAGS::PICKING);
   
           mc->SetModel(steelSphere);
           mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
           tc->GetTransform()->SetScale(0.3f);
           tc->GetTransform()->SetPosition(lightPos);
           bbc->Init();
   
           plc->SetColor({ 0.5f, 0.5f, 0.5f });
           plc->SetIntensity(5.0f);
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
   
       /* ------ Render ------ */
       renderer->Execute();
   }
    
    return EXIT_SUCCESS;
}

Scene* PBRScene(SceneManager* sm)
{
    // Create Scene
    Scene* scene = sm->CreateScene("PBRScene");

    component::CameraComponent* cc = nullptr;
    component::ModelComponent* mc = nullptr;
    component::TransformComponent* tc = nullptr;
    component::InputComponent* ic = nullptr;
    component::BoundingBoxComponent* bbc = nullptr;
    component::PointLightComponent* plc = nullptr;
    component::DirectionalLightComponent* dlc = nullptr;
    component::SpotLightComponent* slc = nullptr;
    component::SkyboxComponent* sbc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* floorModel   = al->LoadModel(L"../Vendor/Assets/Models/FloorPBR/floor.obj");
    Model* boxModel     = al->LoadModel(L"../Vendor/Assets/Models/CubePBR/cube.obj");
    Model* goldenSphere = al->LoadModel(L"../Vendor/Assets/Models/GoldenSphere/sphere.obj");
    Model* steelSphere  = al->LoadModel(L"../Vendor/Assets/Models/SteelSphere/sphere.obj");
    Model* funnyModel   = al->LoadModel(L"../Vendor/Assets/Models/Private/FunnyModel/funnyModel.obj");

    // Load a skybox
    IGraphicsTexture* skyBoxTexture = al->LoadTextureCube(L"../Vendor/Assets/Skyboxes/Skybox_Heaven.dds");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());

    sbc = entity->AddComponent<component::SkyboxComponent>();
    sbc->SetSkyboxTexture(skyBoxTexture);
    /* ---------------------- Player ---------------------- */

    /* ---------------------- FunnyObject ---------------------- */
    entity = scene->AddEntity("FunnyObject");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(funnyModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(7, 7, 7);
    tc->GetTransform()->SetPosition(-4.0f, 0.0f, -6.0f);
    tc->GetTransform()->SetRotationY(PI + PI/8);
    /* ---------------------- FunnyObject ---------------------- */

    /* ---------------------- GoldenSphere ---------------------- */
    entity = scene->AddEntity("GoldenSphere");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(goldenSphere);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(1, 1, 1);
    tc->GetTransform()->SetPosition(6.0f, 2.0f, 8.0f);
    /* ---------------------- GoldenSphere ---------------------- */

    /* ---------------------- SteelSphere ---------------------- */
    entity = scene->AddEntity("SteelSphere");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(steelSphere);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(1, 1, 1);
    tc->GetTransform()->SetPosition(1.0f, 6.0f, -2.5f);
    /* ---------------------- SteelSphere ---------------------- */

    /* ---------------------- Floor ---------------------- */
    entity = scene->AddEntity("floor");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(floorModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(10, 1, 10);
    tc->GetTransform()->SetPosition(0.0f, 0.0f, 0.0f);
    /* ---------------------- Floor ---------------------- */

    /* ---------------------- Spheres ---------------------- */
    auto createSphereLambda = [&scene, steelSphere](std::string name, float3 albedoColor, float3 position, float metallic, float roughness)
    {
        static int index = 0;
        std::string finalName = name + std::to_string(index);
        Entity* entity = scene->AddEntity(finalName);
        component::ModelComponent* mc = entity->AddComponent<component::ModelComponent>();
        component::TransformComponent* tc = entity->AddComponent<component::TransformComponent>();

        mc->SetModel(steelSphere);
        mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);

        AssetLoader* al = AssetLoader::Get();
        std::wstring matName = L"pbrMaterialMainLoop" + std::to_wstring(index);
        Material* newMat = al->CreateMaterial(matName, al->LoadMaterial(mc->GetMaterialAt(0)->GetName()));
        index++;
        MaterialData* matData = newMat->GetSharedMaterialData();
        matData->hasAlbedoTexture = false;
        matData->albedoValue = { albedoColor.x, albedoColor.y, albedoColor.z, 0.0f};

        matData->hasNormalTexture = false;
        matData->hasMetallicTexture = false;
        matData->metallicValue = metallic;
        matData->hasRoughnessTexture = false;
        matData->roughnessValue = roughness;

        mc->SetMaterialAt(0, newMat);

        tc->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);
        tc->GetTransform()->SetPosition(position.x, position.y, position.z);

        tc->Update(0);
        mc->UpdateMaterialRawBufferFromMaterial();
    };

    auto createSpheresLambda = [createSphereLambda](float3 color)
    {
       static unsigned int globalOffset = 0;

       unsigned int length = 7;
       for (unsigned int metallic = 0; metallic < length; metallic++)
       {
          for (unsigned int roughness = 0; roughness < length; roughness++)
          {
             float metallicFloat = (float)metallic / (length - 1);
             float roughnessFloat = max(min((float)roughness / (length - 1), 1.0f), 0.05f);

             std::string name = "Sphere_";
             name.append("M: " + std::to_string(metallicFloat));
             name.append("__R: " + std::to_string(roughnessFloat));

             float localOffset = 1.5f;
             //float3 position = {metallic + offset, roughness + offset, 5.0f};
             float3 position =
             {
                 (roughness - (float(length) / 2)) * localOffset,   // X
                 ((metallic - (float(length) / 2)) * localOffset) + 7,    // Y
                 5.0f + localOffset+ globalOffset // Z
             };
             createSphereLambda(name, color, position, metallicFloat, roughnessFloat);
          }
          int a = 15;
       }

       globalOffset +=5;
    };

    createSpheresLambda({1.0f, 0.0f, 0.0f});
    createSpheresLambda({1.0f, 0.8f, 0.2f});
    createSpheresLambda({0.0f, 1.0f, 0.0f});
    /* ---------------------- Spheres ---------------------- */

    /* ---------------------- PointLights ---------------------- */
    unsigned int lightIntensity = 50.0f;
    entity = scene->AddEntity("pl1");
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    tc->GetTransform()->SetPosition({ -9.0f, 4.4f, -7.0f });
    plc->SetColor({ 1.0f, 1.0f, 1.0f });
    plc->SetIntensity(lightIntensity);

    entity = scene->AddEntity("pl2");
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    tc->GetTransform()->SetPosition({ -9.0f, 17.0f, -7.0f });
    plc->SetColor({ 1.0f, 1.0f, 1.0f });
    plc->SetIntensity(lightIntensity);

    entity = scene->AddEntity("pl3");
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    tc->GetTransform()->SetPosition({ 5.0f, 17.0f, -7.0f});
    plc->SetColor({ 1.0f, 1.0f, 1.0f });
    plc->SetIntensity(lightIntensity);

    entity = scene->AddEntity("pl4");
    tc = entity->AddComponent<component::TransformComponent>();
    plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

    tc->GetTransform()->SetPosition({ 5.0f, 4.4f, -7.0f });
    plc->SetColor({ 1.0f, 1.0f, 1.0f });
    plc->SetIntensity(lightIntensity);
    /* ---------------------- PointLights ---------------------- */

    /* ---------------------- LonelySphere ---------------------- */
    createSphereLambda("LonelySphere", { 1.0f, 0.0f, 1.0f }, { -2.0f, 7.0f, -8.5f }, 0.5f, 0.2f);
    /* ---------------------- LonelySphere ---------------------- */
    
    /* ---------------------- Sun ---------------------- */
    entity = scene->AddEntity("Sun");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(F_LIGHT_FLAGS::CAST_SHADOW);
    dlc->SetColor({ 1.0f, 1.0f, 1.0f });
    dlc->SetIntensity(2.0f);
    dlc->SetDirection({ -0.25f, -0.7f, -0.5f});
    /* ---------------------- Sun ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&PBRUpdateScene);
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
    component::SkyboxComponent* sbc = nullptr;

    AssetLoader* al = AssetLoader::Get();

    // Get the models needed
    Model* sponza = al->LoadModel(L"../Vendor/Assets/Scenes/Sponza/textures_pbr/sponza.obj");
    Model* steelSphere = al->LoadModel(L"../Vendor/Assets/Models/SteelSphere/sphere.obj");
    Model* boxModel = al->LoadModel(L"../Vendor/Assets/Models/CubePBR/cube.obj");
    Model* mirror = al->LoadModel(L"../Vendor/Assets/Models/Mirror/Mirror.obj");

    // Load a skybox
    IGraphicsTexture* skyBoxTexture = al->LoadTextureCube(L"../Vendor/Assets/Skyboxes/Skybox_Heaven.dds");

    /* ---------------------- Player ---------------------- */
    Entity* entity = (scene->AddEntity("player"));
    cc = entity->AddComponent<component::CameraComponent>(E_CAMERA_TYPE::PERSPECTIVE, true);
    ic = entity->AddComponent<component::InputComponent>();
    scene->SetPrimaryCamera(cc->GetCamera());

    sbc = entity->AddComponent<component::SkyboxComponent>();
    sbc->SetSkyboxTexture(skyBoxTexture);
    /* ---------------------- Player ---------------------- */

    /* ---------------------- Mirror1 ---------------------- */
    entity = scene->AddEntity("Mirror1");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(mirror);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(4.0f);
    //tc->GetTransform()->SetRotationX(PI/2 + 0.5);
    //tc->GetTransform()->SetRotationY(PI / 4);
    tc->GetTransform()->SetRotationZ(PI / 2);
    tc->GetTransform()->SetPosition(23, 3.0, 0);

    mc->UpdateMaterialRawBufferFromMaterial();
    bbc->Init();
    /* ---------------------- Mirror1 ---------------------- */

    /* ---------------------- Mirror2 ---------------------- */
    entity = scene->AddEntity("Mirror2");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();
    bbc = entity->AddComponent <component::BoundingBoxComponent>();

    mc->SetModel(mirror);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc->GetTransform()->SetScale(4.0f);
    //tc->GetTransform()->SetRotationX(PI/2 + 0.5);
    tc->GetTransform()->SetRotationY(PI);
    tc->GetTransform()->SetRotationZ(PI / 2);
    tc->GetTransform()->SetPosition(-23, 3.0, 0);

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
    tc->GetTransform()->SetScale(0.02f, 0.02f, 0.02f);
    /* ---------------------- Sponza ---------------------- */

    /* ------------------------ EmissiveSphere --------------------------------- */
    auto createEmissiveSphere = [&scene, steelSphere](std::string name, float4 emissiveColor, float3 position)
    {
        Entity* entity = scene->AddEntity(name);
        component::ModelComponent* mc = entity->AddComponent<component::ModelComponent>();
        component::TransformComponent* tc = entity->AddComponent<component::TransformComponent>();
        component::PointLightComponent* plc = entity->AddComponent<component::PointLightComponent>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION);

        mc->SetModel(steelSphere);
        mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE);

        static int index = 0;
        AssetLoader* al = AssetLoader::Get();
        std::wstring matName = L"dynamicMaterialMainLoop" + std::to_wstring(index);
        Material* newMat = al->CreateMaterial(matName, al->LoadMaterial(mc->GetMaterialAt(0)->GetName()));
        index++;

        MaterialData* matData = newMat->GetSharedMaterialData();
        matData->emissiveValue = emissiveColor;
        matData->hasMetallicTexture = false;
        matData->metallicValue = 0.8f;
        matData->hasRoughnessTexture = false;
        matData->roughnessValue = 0.3f;

        mc->SetMaterialAt(0, newMat);

        plc->SetColor({ emissiveColor.r, emissiveColor.g, emissiveColor.b });
        plc->SetIntensity(emissiveColor.a / 1);

        tc->GetTransform()->SetScale(0.5f, 0.5f, 0.5f);
        tc->GetTransform()->SetPosition(position.x, position.y, position.z);

        mc->UpdateMaterialRawBufferFromMaterial();
        plc->Update(0);
    };


    float4 emissiveColor = { 1.0f, 0.4f, 0.4f, 8.0f };
    float3 position = { 15.0f, 4.0f, -8.0f };
    createEmissiveSphere("RedSphere", emissiveColor, position);

    emissiveColor = { 0.4f, 1.0f, 0.4f, 8.0f };
    position = { 0.0f, 10.0f, 0.0f };
    createEmissiveSphere("GreenSphere", emissiveColor, position);
    
    emissiveColor = { 0.4f, 0.4f, 1.0f, 8.0f };
    position = { 15.0f, 4.0f, 9.0f };
    createEmissiveSphere("BlueSphere", emissiveColor, position);

    /* ---------------------- 1Metre Box ---------------------- */
    entity = scene->AddEntity("Box");
    mc = entity->AddComponent<component::ModelComponent>();
    tc = entity->AddComponent<component::TransformComponent>();

    mc = entity->GetComponent<component::ModelComponent>();
    mc->SetModel(boxModel);
    mc->SetDrawFlag(F_DRAW_FLAGS::DRAW_OPAQUE | F_DRAW_FLAGS::GIVE_SHADOW);
    tc = entity->GetComponent<component::TransformComponent>();
    tc->GetTransform()->SetScale(1, 1, 1);
    tc->GetTransform()->SetPosition(0.0f, 5.0f, 5.0f);
    /* ---------------------- 1Metre Box ---------------------- */

    /* ---------------------- Sun ---------------------- */
    entity = scene->AddEntity("Sun");
    dlc = entity->AddComponent<component::DirectionalLightComponent>(F_LIGHT_FLAGS::CAST_SHADOW);
    dlc->SetColor({ 1.0f, 1.0f, 1.0f });
    dlc->SetIntensity(2.0f);
    dlc->SetDirection({ -0.6f, -1.0f, 0.05f});
    /* ---------------------- Sun ---------------------- */

    /* ---------------------- Update Function ---------------------- */
    scene->SetUpdateScene(&SponzaUpdateScene);
    return scene;
}

void PBRUpdateScene(SceneManager* sm, double dt)
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
    // Currently just some movement on three lights
    component::TransformComponent* tcRed    = sm->GetScene("SponzaScene")->GetEntity("RedSphere")->GetComponent<component::TransformComponent>();
    component::TransformComponent* tcGreen  = sm->GetScene("SponzaScene")->GetEntity("GreenSphere")->GetComponent<component::TransformComponent>();
    component::TransformComponent* tcBlue   = sm->GetScene("SponzaScene")->GetEntity("BlueSphere")->GetComponent<component::TransformComponent>();
    Transform* tRed     = tcRed->GetTransform();
    Transform* tGreen   = tcGreen->GetTransform();
    Transform* tBlue    = tcBlue->GetTransform();

    float3 currPosRed   = tRed->GetPositionFloat3();
    float3 currPosGreen = tGreen->GetPositionFloat3();
    float3 currPosBlue  = tBlue->GetPositionFloat3();

    static float zPosRedSphere      = currPosRed.z;
    static float zPosGreenSphere    = currPosGreen.z;
    static float yPosGreenSphere    = currPosGreen.y;
    static float zPosBlueSphere     = currPosBlue.z;

    float movedZPosRedSphere    = sinf(zPosRedSphere) * 20;
    float movedZPosGreenSphere  = sinf(zPosGreenSphere) * 20;
    float movedYPosGreenSphere  = abs(sinf(yPosGreenSphere)) * 5;
    float movedZPosBlueSphere   = sinf(zPosBlueSphere) * 20;

    // Set position
    tRed->SetPosition(movedZPosRedSphere, currPosRed.y, currPosRed.z);
    tGreen->SetPosition(movedZPosGreenSphere, movedYPosGreenSphere, currPosGreen.z);
    tBlue->SetPosition(movedZPosBlueSphere, currPosBlue.y, currPosBlue.z);

    // Animate
    zPosGreenSphere += 0.5f * dt;
    yPosGreenSphere += 1.9f * dt;
    zPosRedSphere   += 0.2f * dt;
    zPosBlueSphere  += 1.0f * dt;
}
