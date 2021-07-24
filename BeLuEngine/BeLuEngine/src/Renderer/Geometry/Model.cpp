#include "stdafx.h"
#include "GPU_Structs.h"
#include "Model.h"

#include "../Misc/Log.h"

#include "Mesh.h"
#include "Material.h"
#include "../Texture/Texture2D.h"
#include "../GPUMemory/GPUMemory.h"

#include "../DXR/BottomLevelAccelerationStructure.h"

Model::Model(
	const std::wstring* path,
	std::vector<Mesh*>* meshes,
	std::vector<Material*>* materials,
	ID3D12Device5* pdevice)
{
	m_Path = *path;
	m_Size = (*meshes).size();

	m_Meshes = (*meshes);
	m_OriginalMaterial = *materials;

	// DXR
	m_pBLAS = new BottomLevelAccelerationStructure();

	for (const Mesh* mesh : m_Meshes)
	{
		m_pBLAS->AddVertexBuffer(
			mesh->GetDefaultResourceVertices(), mesh->GetNumVertices(),
			mesh->GetDefaultResourceIndices(), mesh->GetNumIndices());
	}

	m_pBLAS->GenerateBuffers(pdevice);
	m_pBLAS->SetupAccelerationStructureForBuilding(pdevice, false);
}

Model::~Model()
{
	SAFE_DELETE(m_pBLAS);
}

bool Model::operator==(const Model& other)
{
	return m_Path == other.m_Path;
}

bool Model::operator!=(const Model& other)
{
	return !(operator==(other));
}

const std::wstring& Model::GetPath() const
{
	return m_Path;
}

unsigned int Model::GetSize() const
{
	return m_Size;
}

Mesh* Model::GetMeshAt(unsigned int index) const
{
	return m_Meshes[index];
}

const std::vector<Material*>* Model::GetOriginalMaterial() const
{
	return &m_OriginalMaterial;
}
