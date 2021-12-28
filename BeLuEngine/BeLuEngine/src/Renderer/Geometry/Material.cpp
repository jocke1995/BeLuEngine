#include "stdafx.h"
#include "Material.h"

#include "../Misc/AssetLoader.h"
#include "../Renderer/Renderer.h"

#include "../API/IGraphicsTexture.h"

Material::Material(const std::wstring& name)
{
	AssetLoader* al = AssetLoader::Get();
	
	Material* defaultMat = al->LoadMaterial(L"DefaultMaterial");
	
	BL_ASSERT_MESSAGE(defaultMat != nullptr, "Trying to create a material from the default material when it has not been created yet.\n");

	m_MaterialData =
	{
			defaultMat->m_MaterialData.textureAlbedo,
			defaultMat->m_MaterialData.textureRoughness,
			defaultMat->m_MaterialData.textureMetallic,
			defaultMat->m_MaterialData.textureNormal,
			defaultMat->m_MaterialData.textureEmissive,
			defaultMat->m_MaterialData.textureOpacity,
			0,	// useEmissiveTexture
			0,	// useRoughnessTexture
			0,	// useMetallicTexture
			0,	// useOpacityTexture
			0,	// useNormalTexture
			0,  // Glow
			{0.0f, 0.0f, 0.0f, 1.0f}, // EmissiveColor + strength
			0.5f, // roughnessValue
			0.5f, // metallicValue
			0.5f,  // opacityValue
			0, // padding
	};

	// copy the texture pointers
	m_Textures = defaultMat->m_Textures;
}

Material::Material(const std::wstring* name, std::map<E_TEXTURE2D_TYPE, IGraphicsTexture*>* textures)
{
	m_Name = *name;

	m_MaterialData = 
	{	
			textures->at(E_TEXTURE2D_TYPE::ALBEDO)->GetShaderResourceHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::ROUGHNESS)->GetShaderResourceHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::METALLIC)->GetShaderResourceHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::NORMAL)->GetShaderResourceHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::EMISSIVE)->GetShaderResourceHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::OPACITY)->GetShaderResourceHeapIndex(),
			1,	// useEmissiveTexture
			1,	// useRoughnessTexture
			1,	// useMetallicTexture
			0,	// useOpacityTexture
			1,	// useNormalTexture
			0,  // Glow
			{0.0f, 0.0f, 0.0f, 1.0f}, // EmissiveColor + strength
			0.01f, // roughnessValue
			0.01f, // metallicValue
			0.5f,  // opacityValue
			0, // padding
	};
	// copy the texture pointers
	m_Textures = *textures;
}

Material::Material(const Material& other, const std::wstring& name)
{
	m_Name = name;
	m_MaterialData = other.m_MaterialData;
	m_Textures = other.m_Textures;
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

IGraphicsTexture* Material::GetTexture(E_TEXTURE2D_TYPE type) const
{
	return m_Textures.at(type);
}

MaterialData* Material::GetSharedMaterialData()
{
	return &m_MaterialData;
}

void Material::SetTexture(E_TEXTURE2D_TYPE type, IGraphicsTexture* texture)
{
	m_Textures[type] = texture;
}
