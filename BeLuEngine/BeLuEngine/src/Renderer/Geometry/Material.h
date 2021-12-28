#ifndef MATERIAL_H
#define MATERIAL_H

#include "GPU_Structs.h"

class IGraphicsTexture;
class D3D12DescriptorHeap;

class Material
{
public:
    Material(const std::wstring& name); // Default
    Material(const std::wstring* path, std::map<E_TEXTURE2D_TYPE, IGraphicsTexture*>* textures); // From parameters (used when loading objects from files)
    Material(const Material& other, const std::wstring& name); // Copy constructor
    virtual ~Material();

    bool operator == (const Material& other);
    bool operator != (const Material& other);

    const std::wstring& GetPath() const;
    std::wstring GetName() const;

    // Material
    IGraphicsTexture* GetTexture(E_TEXTURE2D_TYPE type) const;
    MaterialData* GetSharedMaterialData();

    void SetTexture(E_TEXTURE2D_TYPE type, IGraphicsTexture* texture);

private:
    friend class Renderer;

    std::wstring m_Name;
    std::map<E_TEXTURE2D_TYPE, IGraphicsTexture*> m_Textures;
    MaterialData m_MaterialData = {};
    
};

#endif
