#ifndef MODEL_H
#define MODEL_H

class Mesh;
class Material;
struct SlotInfo;

// DXR
class IBottomLevelAS;

class Model
{
public:
    Model(const std::wstring* path,
        std::vector<Mesh*>* meshes,
        std::vector<Material*>* materials);
    virtual ~Model();

    bool operator == (const Model& other);
    bool operator != (const Model& other);

    const std::wstring& GetPath() const;
    unsigned int GetSize() const;

    // Mesh
    Mesh* GetMeshAt(unsigned int index) const;

    const std::vector<Material*>* GetOriginalMaterial() const;

protected:
    friend class Renderer;
    friend class AssetLoader;
    friend class TopLevelRenderTask;

    std::wstring m_Path;
    unsigned int m_Size = 0;

    std::vector<Mesh*> m_Meshes;
    std::vector<Material*> m_OriginalMaterial;

    // DXR
    IBottomLevelAS* m_pBLAS = nullptr;
};

#endif
