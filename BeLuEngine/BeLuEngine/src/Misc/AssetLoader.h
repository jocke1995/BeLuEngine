#ifndef ASSETLOADER_H
#define ASSETLOADER_H

#include "Core.h"
#include "assimp/matrix4x4.h"
#include <map>

class DescriptorHeap;
class Model;
class Mesh;
class Shader;
class Texture;
class TextureCubeMap;
class Texture2DGUI;
class Material;
class Window;
class Scene;

struct ID3D12Device5;
struct Vertex;
struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;


class AssetLoader
{
public:
    ~AssetLoader();

    static AssetLoader* Get(ID3D12Device5* device = nullptr, DescriptorHeap* descriptorHeap_CBV_UAV_SRV = nullptr, const Window* window = nullptr);

    /* Load Functions */
    // Model ---------------
    Model* LoadModel(const std::wstring& path);
    // Textures ------------
    Texture* LoadTexture2D(const std::wstring& path);

    // IsLoadedFunctions
    bool IsModelLoadedOnGpu(const std::wstring& name) const;
    bool IsModelLoadedOnGpu(const Model* model) const;
    bool IsMaterialLoadedOnGpu(const std::wstring& name) const;
    bool IsMaterialLoadedOnGpu(const Material* material) const;
    bool IsTextureLoadedOnGpu(const std::wstring& name) const;
    bool IsTextureLoadedOnGpu(const Texture* texture) const;

private:
    // PipelineState loads all shaders
    friend class PipelineState;
    // Renderer needs access to m_LoadedModels & m_LoadedTextures so it can check if they are uploaded to GPU.
    friend class Renderer;

    // Constructor currently called from m_pRenderer to set dx12 specific objects
    AssetLoader(ID3D12Device5* device = nullptr, DescriptorHeap* descriptorHeap_CBV_UAV_SRV = nullptr, const Window* window = nullptr);
    AssetLoader(AssetLoader const&) = delete;
    void operator=(AssetLoader const&) = delete;

    ID3D12Device5* m_pDevice = nullptr;
    DescriptorHeap* m_pDescriptorHeap_CBV_UAV_SRV = nullptr;
    Window* m_pWindow = nullptr;
    
    void loadDefaultMaterial();

    const std::wstring m_FilePathShaders = L"../BeLuEngine/src/Renderer/HLSL/";
    const std::wstring m_FilePathDefaultTextures = L"../Vendor/Resources/Textures/Default/";

    // Every model & texture also has a bool which indicates if its data is on the GPU or not
    // name, pair<isOnGpu, Model*>
    std::map<std::wstring, std::pair<bool, Model*>> m_LoadedModels;
    std::map<std::wstring, std::pair<bool, Material*>> m_LoadedMaterials;
    std::map<std::wstring, std::pair<bool, Texture*>> m_LoadedTextures;
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
    Texture* processTexture(aiMaterial* mat, TEXTURE2D_TYPE texture_type, const std::wstring& filePathWithoutTexture);
   
    Shader* loadShader(const std::wstring& fileName, ShaderType type);
};

#endif