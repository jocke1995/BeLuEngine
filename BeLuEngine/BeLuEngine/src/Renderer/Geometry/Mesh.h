#ifndef MESH_H
#define MESH_H

#include "../ECS/Components/BoundingBoxComponent.h"

class Texture;
struct SlotInfo;

class IGraphicsBuffer;

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
    virtual void Init();

    // Vertices
    const std::vector<Vertex>* GetVertices() const;
    virtual const unsigned int GetSizeOfVertices() const;
    virtual const unsigned int GetNumVertices() const;

    // Indices
    const std::vector<unsigned int>* GetIndices() const;
    virtual const unsigned int GetSizeOfIndices() const;
    virtual const unsigned int GetNumIndices() const;

    const std::wstring& GetPath() const;

    IGraphicsBuffer* GetVertexBuffer() const;
    IGraphicsBuffer* GetIndexBuffer() const;

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

    IGraphicsBuffer* m_pVertexBuffer = nullptr;
    IGraphicsBuffer* m_pIndexBuffer = nullptr;

    unsigned int m_Id = 0;
};

#endif
