#include "stdafx.h"
#include "Material.h"

Material::Material(const std::wstring* name, std::map<E_TEXTURE2D_TYPE, Texture*>* textures, MaterialData matData)
{
	m_Name = *name;

	m_MaterialData = matData;
	// copy the texture pointers
	m_Textures = *textures;
}

Material::~Material()
{

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

const MaterialData* Material::GetMaterialData() const
{
	return &m_MaterialData;
}

void Material::SetTexture(E_TEXTURE2D_TYPE type, Texture* texture)
{
	m_Textures[type] = texture;
}
