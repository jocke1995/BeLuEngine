#include "stdafx.h"
#include "GPU_Structs.h"
#include "Model.h"

#include "../Misc/Log.h"


#include "Mesh.h"
#include "Material.h"
#include "../Texture/Texture2D.h"
#include "../GPUMemory/GPUMemory.h"

Model::Model(const std::wstring* path, std::vector<Mesh*>* meshes, std::vector<Material*>* materials)
{
	m_Path = *path;
	m_Size = (*meshes).size();

	m_Meshes = (*meshes);
	m_OriginalMaterial = *materials;
}

Model::~Model()
{
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
