#ifndef MATERIAL_H
#define MATERIAL_H

class Texture;
class Resource;
class ShaderResourceView;
class DescriptorHeap;

// DX12 Forward Declarations
struct ID3D12Device5;
struct D3D12_INDEX_BUFFER_VIEW;

class Material
{
public:
    Material(const std::wstring* path, std::map<E_TEXTURE2D_TYPE, Texture*>* textures);
    virtual ~Material();

    bool operator == (const Material& other);
    bool operator != (const Material& other);

    const std::wstring& GetPath() const;

    // Material
    Texture* GetTexture(E_TEXTURE2D_TYPE type) const;

    void SetTexture(E_TEXTURE2D_TYPE type, Texture* texture);

private:
    friend class Renderer;

    std::wstring m_Name;
    std::map<E_TEXTURE2D_TYPE, Texture*> m_Textures;
};

#endif
