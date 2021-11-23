#ifndef MESH_H
#define MESH_H

#include "../ECS/Components/BoundingBoxComponent.h"

class Texture;
class Resource;
class ShaderResourceView;
class DescriptorHeap;
struct SlotInfo;

// DX12 Forward Declarations
struct ID3D12Device5;
struct D3D12_INDEX_BUFFER_VIEW;

struct Vertex
{
    DirectX::XMFLOAT3 pos;
    DirectX::XMFLOAT2 uv;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
};

static unsigned int s_IdCounter = 0;

class Mesh
{
public:
    Mesh(   std::vector<Vertex>* vertices,
            std::vector<unsigned int>* indices,
            const std::wstring& path = L"NOPATH");
    virtual ~Mesh();

    bool operator == (const Mesh& other);
    bool operator != (const Mesh& other);

    // Virtual so that animatedMesh can override this
    virtual void Init(ID3D12Device5* m_pDevice5, DescriptorHeap* CBV_UAV_SRV_heap);

    // Vertices
    Resource* GetDefaultResourceVertices() const;
    const std::vector<Vertex>* GetVertices() const;
    virtual const unsigned int GetSizeOfVertices() const;
    virtual const unsigned int GetNumVertices() const;

    // Indices
    Resource* GetDefaultResourceIndices() const;
    const std::vector<unsigned int>* GetIndices() const;
    virtual const unsigned int GetSizeOfIndices() const;
    virtual const unsigned int GetNumIndices() const;
    const D3D12_INDEX_BUFFER_VIEW* GetIndexBufferView() const;

    const std::wstring& GetPath() const;

	ShaderResourceView* const GetVBSRV() const;
	ShaderResourceView* const GetIBSRV() const;

protected:
    friend class MergeRenderTask;
    friend class DownSampleRenderTask;
    friend class DeferredLightRenderTask;

    friend class Renderer;
    friend class AssetLoader;
    friend class SceneManager;
	friend class Model;
	friend class component::BoundingBoxComponent;
	friend class CopyOnDemandTask;

    std::vector<Vertex> m_Vertices;
    std::vector<unsigned int> m_Indices;
    std::wstring m_Path = L"NOPATH";

    Resource* m_pUploadResourceVertices = nullptr;
    Resource* m_pUploadResourceIndices = nullptr;
    Resource* m_pDefaultResourceVertices = nullptr;
    Resource* m_pDefaultResourceIndices = nullptr;

    ShaderResourceView* m_pVertexBufferSRV = nullptr;
    ShaderResourceView* m_pIndexBufferSRV = nullptr;

    D3D12_INDEX_BUFFER_VIEW* m_pIndexBufferView = nullptr;

    unsigned int m_Id = 0;
};

#endif
