#include "stdafx.h"
#include "Mesh.h"

#include "../API/IGraphicsBuffer.h"

Mesh::Mesh(std::vector<Vertex>* vertices, std::vector<unsigned int>* indices, const std::wstring& path)
{
	m_Id = s_IdCounter++;

	m_Path = path + L"_Mesh_ID_" + std::to_wstring(m_Id);
	m_Vertices = *vertices;
	m_Indices = *indices;
}

Mesh::~Mesh()
{
	BL_SAFE_DELETE(m_pVertexBuffer);
	BL_SAFE_DELETE(m_pIndexBuffer);
}

bool Mesh::operator==(const Mesh& other)
{
	return m_Id == other.m_Id;
}

bool Mesh::operator!=(const Mesh& other)
{
	return !(operator==(other));
}

void Mesh::Init()
{
	std::string modelPathName = to_string(m_Path);
	modelPathName = modelPathName.substr(modelPathName.find_last_of("/\\") + 1);

	// create vertices resource
	m_pVertexBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::VertexBuffer, E_GRAPHICSBUFFER_UPLOADFREQUENCY::Static, sizeof(Vertex), m_Vertices.size(), DXGI_FORMAT_UNKNOWN, to_wstring(modelPathName) + L"_VERTEXBUFFER");
	m_pIndexBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::IndexBuffer, E_GRAPHICSBUFFER_UPLOADFREQUENCY::Static, sizeof(unsigned int), m_Indices.size(), DXGI_FORMAT_UNKNOWN, to_wstring(modelPathName) + L"_INDEXBUFFER");

	// Set indexBufferView
	//m_pIndexBufferView = new D3D12_INDEX_BUFFER_VIEW();
	//m_pIndexBufferView->BufferLocation = m_pIndexBuffer->GetGPUVirtualAdress();
	//m_pIndexBufferView->Format = DXGI_FORMAT_R32_UINT;
	//m_pIndexBufferView->SizeInBytes = GetSizeOfIndices();
}

const std::vector<Vertex>* Mesh::GetVertices() const
{
	return &m_Vertices;
}

const unsigned int Mesh::GetSizeOfVertices() const
{
	return m_Vertices.size() * sizeof(Vertex);
}

const unsigned int Mesh::GetNumVertices() const
{
	return m_Vertices.size();
}

const std::vector<unsigned int>* Mesh::GetIndices() const
{
	return &m_Indices;
}

const unsigned int Mesh::GetSizeOfIndices() const
{
	return m_Indices.size() * sizeof(unsigned int);
}

const unsigned int Mesh::GetNumIndices() const
{
	return m_Indices.size();
}

const std::wstring& Mesh::GetPath() const
{
	return m_Path;
}

IGraphicsBuffer* Mesh::GetVertexBuffer() const
{
	return m_pVertexBuffer;
}

IGraphicsBuffer* Mesh::GetIndexBuffer() const
{
	return m_pIndexBuffer;
}
