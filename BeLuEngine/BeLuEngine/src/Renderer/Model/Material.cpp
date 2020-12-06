#include "stdafx.h"
#include "Material.h"

Material::Material(const std::wstring* name, std::map<TEXTURE2D_TYPE, Texture*>* textures)
{
	m_Name = *name;

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

Texture* Material::GetTexture(TEXTURE2D_TYPE type) const
{
	return m_Textures.at(type);
}

void Material::SetTexture(TEXTURE2D_TYPE type, Texture* texture)
{
	m_Textures[type] = texture;
}
