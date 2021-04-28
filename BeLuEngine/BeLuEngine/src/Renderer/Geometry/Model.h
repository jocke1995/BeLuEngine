#ifndef MODEL_H
#define MODEL_H

class Mesh;
class Material;
class Resource;
class ShaderResourceView;
class DescriptorHeap;
struct SlotInfo;

// DX12 Forward Declarations
struct ID3D12Device5;
struct D3D12_INDEX_BUFFER_VIEW;

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

    std::wstring m_Path;
    unsigned int m_Size = 0;

    std::vector<Mesh*> m_Meshes;
    std::vector<Material*> m_OriginalMaterial;
};

#endif
