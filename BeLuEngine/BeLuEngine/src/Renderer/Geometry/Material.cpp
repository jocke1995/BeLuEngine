#include "stdafx.h"
#include "Material.h"

#include "../Misc/Log.h"
#include "../Misc/AssetLoader.h"

#include "../Texture/Texture.h"

#include "../Renderer/Renderer.h"
#include "../Renderer/DescriptorHeap.h"

#include "../Renderer/GPUMemory/GPUMemory.h"
Material::Material(const std::wstring& name)
{
	AssetLoader* al = AssetLoader::Get();
	
	Material* defaultMat = al->LoadMaterial(L"DefaultMaterial");
	
#ifdef DEBUG
	// Sanity check
	if (defaultMat == nullptr)
	{
		BL_LOG_CRITICAL("Trying to create a material from the default material when it has not been created yet.\n");
	}
#endif 

	Renderer& r = Renderer::GetInstance();
	m_MaterialData.first = new ConstantBuffer(r.m_pDevice5, sizeof(MaterialData), m_Name, r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
	m_MaterialData.second =
	{
			defaultMat->m_MaterialData.second.textureAlbedo,
			defaultMat->m_MaterialData.second.textureRoughness,
			defaultMat->m_MaterialData.second.textureMetallic,
			defaultMat->m_MaterialData.second.textureNormal,
			defaultMat->m_MaterialData.second.textureEmissive,
			defaultMat->m_MaterialData.second.textureOpacity,
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

Material::Material(const std::wstring* name, std::map<E_TEXTURE2D_TYPE, Texture*>* textures)
{
	m_Name = *name;

	Renderer& r = Renderer::GetInstance();
	m_MaterialData.first = new ConstantBuffer(r.m_pDevice5, sizeof(MaterialData), m_Name, r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
	m_MaterialData.second = 
	{	
			textures->at(E_TEXTURE2D_TYPE::ALBEDO)->GetDescriptorHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::ROUGHNESS)->GetDescriptorHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::METALLIC)->GetDescriptorHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::NORMAL)->GetDescriptorHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::EMISSIVE)->GetDescriptorHeapIndex(),
			textures->at(E_TEXTURE2D_TYPE::OPACITY)->GetDescriptorHeapIndex(),
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

	Renderer& r = Renderer::GetInstance();
	m_MaterialData.first = new ConstantBuffer(r.m_pDevice5, sizeof(MaterialData), m_Name, r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

	m_MaterialData.second = other.m_MaterialData.second;


	m_Textures = other.m_Textures;
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