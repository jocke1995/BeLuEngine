#include "stdafx.h"
#include "GPU_Structs.h"
#include "Model.h"

#include "Mesh.h"
#include "Material.h"
#include "../Texture/Texture2D.h"
#include "../GPUMemory/GPUMemory.h"

Model::Model(const std::wstring* path, std::vector<Mesh*>* meshes, std::vector<Material*>* materials)
{
	m_Path = *path;
	m_Size = (*meshes).size();

	m_Meshes = (*meshes);
	m_Materials = (*materials);

	// Fill slotinfo with empty slotinfos
	m_SlotInfos.resize(m_Size);
	updateSlotInfo();
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

void Model::SetMeshAt(unsigned int index, Mesh* mesh)
{
	m_Meshes[index] = mesh;
	updateSlotInfo();
}

Material* Model::GetMaterialAt(unsigned int index) const
{
	return m_Materials[index];
}

void Model::SetMaterialAt(unsigned int index, Material* material)
{
	m_Materials[index] = material;
	updateSlotInfo();
}

const SlotInfo* Model::GetSlotInfoAt(unsigned int index) const
{
	return &m_SlotInfos[index];
}

void Model::updateSlotInfo()
{
#ifdef _DEBUG
	if (m_Meshes[0]->m_pSRV == nullptr || m_Materials[0]->GetTexture(TEXTURE2D_TYPE::ALBEDO)->m_pSRV == nullptr)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Model.cpp::updateSlotInfo got unInit:ed variables\n");
	}
#endif // DEBUG

	for (unsigned int i = 0; i < m_Size; i++)
	{
		m_SlotInfos[i] =
		{
		m_Meshes[i]->m_pSRV->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::ALBEDO)->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::ROUGHNESS)->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::METALLIC)->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::NORMAL)->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::EMISSIVE)->GetDescriptorHeapIndex(),
		m_Materials[i]->GetTexture(TEXTURE2D_TYPE::OPACITY)->GetDescriptorHeapIndex()
		};
	}
}

