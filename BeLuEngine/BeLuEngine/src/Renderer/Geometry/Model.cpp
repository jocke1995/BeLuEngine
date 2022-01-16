#include "stdafx.h"
#include "Model.h"

// Geometry
#include "Mesh.h"
#include "Material.h"

#include "../API/Interface/RayTracing/IBottomLevelAS.h"

Model::Model(
	const std::wstring* path,
	std::vector<Mesh*>* meshes,
	std::vector<Material*>* materials)
{
	m_Path = *path;
	m_Size = (*meshes).size();

	m_Meshes = (*meshes);
	m_OriginalMaterial = *materials;

	// DXR
	m_pBLAS = IBottomLevelAS::Create();

	for (const Mesh* mesh : m_Meshes)
	{
		m_pBLAS->AddGeometry(
			mesh->GetVertexBuffer(), mesh->GetNumVertices(),
			mesh->GetIndexBuffer(), mesh->GetNumIndices());
	}

	m_pBLAS->GenerateBuffers();
	m_pBLAS->SetupAccelerationStructureForBuilding();
}

Model::~Model()
{
	BL_SAFE_DELETE(m_pBLAS);
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
