#ifndef ASSETLOADER_H
#define ASSETLOADER_H

#include "Core.h"
#include "RenderCore.h"

#include "assimp/matrix4x4.h"
#include <map>

class Model;
class Mesh;
class Shader;
class Material;
class Window;
class Scene;

// Assimp
struct Vertex;
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

// API
class IGraphicsTexture;

struct DXILCompilationDesc;

static inline const std::wstring s_FilePathShaders = L"../BeLuEngine/src/Renderer/Shaders/HLSL/";
static inline const std::wstring s_FilePathDefaultTextures = L"../Vendor/Assets/Textures/Default/";

class AssetLoader
{
public:
    ~AssetLoader();

    static AssetLoader* Get();

    bool DeleteAllAssets();

    /* Load Functions */
    // Model ---------------
    Model* LoadModel(const std::wstring& path);

    // Textures ------------
    IGraphicsTexture* LoadTexture2D(E_TEXTURE2D_TYPE textureType, const std::wstring& path);
    IGraphicsTexture* LoadTextureCube(const std::wstring& path);

    // Creates material from parameter.
    // If no parameter is specified, a default material is created.
    Material* CreateMaterial(std::wstring matName, const Material* mat = nullptr);
    Material* LoadMaterial(std::wstring matName);

    // IsLoadedFunctions
    bool IsModelLoadedOnGpu(const std::wstring& name) const;
    bool IsModelLoadedOnGpu(const Model* model) const;
    bool IsTextureLoadedOnGpu(const std::wstring& name) const;
    bool IsTextureLoadedOnGpu(const IGraphicsTexture* texture) const;

private:
    friend class D3D12GraphicsPipelineState;;
    friend class RayTracingPipelineStateDesc;  // This class loads ray-tracing shaders

    friend class Renderer; // Renderer needs access to m_LoadedModels & m_LoadedTextures so it can check if they are uploaded to GPU.

    AssetLoader();
    AssetLoader(AssetLoader const&) = delete;
    void operator=(AssetLoader const&) = delete;

    Window* m_pWindow = nullptr;
    
    void loadDefaultMaterial();

    // Every model & texture also has a bool which indicates if its data is on the GPU or not
    // name, pair<isOnGpu, Model*>
    std::map<std::wstring, std::pair<bool, Model*>> m_LoadedModels;
    std::map<std::wstring, std::pair<bool, IGraphicsTexture*>> m_LoadedTextures;
    std::map<std::wstring, Material*> m_LoadedMaterials;
    std::vector<Mesh*> m_LoadedMeshes;
    std::map<std::wstring, Shader*> m_LoadedShaders;

    /* --------------- Functions --------------- */
    void processModel(const aiScene* assimpScene,
        std::vector<Mesh*>* meshes,
        std::vector<Material*>* materials,
        const std::wstring& filePath);

    Mesh* processMesh(aiMesh* assimpMesh,
        const aiScene* assimpScene,
        std::vector<Mesh*>* meshes,
        std::vector<Material*>* materials,
        const std::wstring& filePath);

    void processMeshData(const aiScene* assimpScene, const aiMesh* assimpMesh, std::vector<Vertex>* vertices, std::vector<unsigned int>* indices);
    Material* processMaterial(std::wstring path, const aiScene* assimpScene, const aiMesh* assimpMesh);
    Material* loadMaterial(aiMaterial* mat, const std::wstring& folderPath);
    IGraphicsTexture* processTexture(aiMaterial* mat, E_TEXTURE2D_TYPE texture_type, const std::wstring& filePathWithoutTexture);
   
    // Loads a shader and appends the entire filePath from the fileName
    Shader* loadShader(DXILCompilationDesc* desc);
};

#endif