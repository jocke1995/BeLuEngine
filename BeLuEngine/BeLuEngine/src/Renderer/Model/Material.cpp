#include "stdafx.h"
#include "Material.h"

#include "../Renderer/Renderer.h"
#include "../Renderer/DescriptorHeap.h"

#include "../Renderer/GPUMemory/GPUMemory.h"
Material::Material(const std::wstring* name, std::map<E_TEXTURE2D_TYPE, Texture*>* textures)
{
	m_Name = *name;

	Renderer& r = Renderer::GetInstance();
	m_MaterialData.first = new ConstantBuffer(r.m_pDevice5, sizeof(MaterialData), m_Name, r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
	m_MaterialData.second = {0.0f, 1.0f, 0.0f, 0.0f};
	// copy the texture pointers
	m_Textures = *textures;
}

Material::~Material()
{
	delete m_MaterialData.first;
}

bool Material::operator==(const Material& other)
{
	return m_Name == other.m_Name;
}

bool Material::operator!=(const Material& other)
{
	return !(operator==(other));
}

const std::wstring& Material::GetPath() const
{
	return m_Name;
}

std::wstring Material::GetName() const
{
	return m_Name;
}

Texture* Material::GetTexture(E_TEXTURE2D_TYPE type) const
{
	return m_Textures.at(type);
}

const std::pair<ConstantBuffer*, MaterialData>* Material::GetMaterialData() const
{
	return &m_MaterialData;
}

void Material::SetTexture(E_TEXTURE2D_TYPE type, Texture* texture)
{
	m_Textures[type] = texture;
}
