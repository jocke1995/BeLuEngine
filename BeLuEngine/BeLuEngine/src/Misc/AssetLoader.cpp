#include "stdafx.h"
#include "AssetLoader.h"

#include "../Renderer/Material.h"

#include "../Renderer/DescriptorHeap.h"

#include "../Renderer/Mesh.h"
#include "../Renderer/Shader.h"
#include "../Renderer/Texture.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

AssetLoader::AssetLoader(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV)
{
	m_pDevice = device;
	m_pDescriptorHeap_CBV_UAV_SRV = descriptorHeap_CBV_UAV_SRV;

	// Load default textures
	LoadTexture(m_FilePathDefaultTextures + L"default_ambient.png");
	LoadTexture(m_FilePathDefaultTextures + L"default_diffuse.jpg");
	LoadTexture(m_FilePathDefaultTextures + L"default_specular.png");
	LoadTexture(m_FilePathDefaultTextures + L"default_normal.png");
	LoadTexture(m_FilePathDefaultTextures + L"default_emissive.png");
}

AssetLoader::~AssetLoader()
{
	// For every model
	for (auto pair : m_LoadedModels)
	{
		// For every m_pMesh the model has
		for (unsigned int i = 0; i < pair.second.second->size(); i++)
		{
			delete pair.second.second->at(i);
		}
		delete pair.second.second;
	}

	// For every texture
	for (auto pair : m_LoadedTextures)
	{
		delete pair.second.second;
	}

	// For every shader
	for (auto shader : m_LoadedShaders)
		delete shader.second;
}

AssetLoader* AssetLoader::Get(ID3D12Device5* device, DescriptorHeap* descriptorHeap_CBV_UAV_SRV)
{
	static AssetLoader instance(device, descriptorHeap_CBV_UAV_SRV);

	return &instance;
}

std::vector<Mesh*>* AssetLoader::LoadModel(const std::wstring path)
{
	// Check if the model already exists
	if (m_LoadedModels.count(path) != 0)
	{
		return m_LoadedModels[path].second;
	}

	// Else load the model
	const std::string filePath(path.begin(), path.end());
	Assimp::Importer importer;

	const aiScene* assimpScene = importer.ReadFile(filePath, aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_GenUVCoords | aiProcess_CalcTangentSpace);

	if (assimpScene == nullptr)
	{
		Log::PrintSeverity(Log::Severity::CRITICAL, "Failed to load model with path: \'%s\'\n", filePath.c_str());
		return nullptr;
	}
	
	std::vector<Mesh*> *meshes = new std::vector<Mesh*>;
	meshes->reserve(assimpScene->mNumMeshes);
	m_LoadedModels[path].first = false;
	m_LoadedModels[path].second = meshes;

	processNode(assimpScene->mRootNode, assimpScene, meshes, &filePath);

	return m_LoadedModels[path].second;
}

Texture* AssetLoader::LoadTexture(std::wstring path)
{
	// Check if the texture already exists
	if (m_LoadedTextures.count(path) != 0)
	{
		return m_LoadedTextures[path].second;
	}

	Texture* texture = new Texture();
	if (texture->Init(path, m_pDevice, m_pDescriptorHeap_CBV_UAV_SRV) == false)
	{
		delete texture;
		return nullptr;
	}

	m_LoadedTextures[path].first = false;
	m_LoadedTextures[path].second = texture;
	return texture;
}

Shader* AssetLoader::loadShader(std::wstring fileName, ShaderType type)
{
	// Check if the shader already exists
	if (m_LoadedShaders.count(fileName) != 0)
	{
		return m_LoadedShaders[fileName];
	}
	// else, create a new shader and compile it

	std::wstring entireFilePath = m_FilePathShaders + fileName;
	Shader* tempShader = new Shader(entireFilePath.c_str(), type);

	m_LoadedShaders[fileName] = tempShader;
	return m_LoadedShaders[fileName];
}

void AssetLoader::processNode(aiNode* node, const aiScene* assimpScene, std::vector<Mesh*>* meshes, const std::string* filePath)
{
	// Go through all the m_Meshes
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = assimpScene->mMeshes[node->mMeshes[i]];
		meshes->push_back(processMesh(mesh, assimpScene, filePath));
	}
	
	// If the node has more node children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], assimpScene, meshes, filePath);
	}
}

Mesh* AssetLoader::processMesh(aiMesh* assimpMesh, const aiScene* assimpScene, const std::string* filePath)
{
	// Fill this data
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::map<TEXTURE_TYPE, Texture*> textures;

	// Get data from assimpMesh and store it
	for (unsigned int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		Vertex vTemp = {};

		// Get positions
		if (assimpMesh->HasPositions())
		{
			vTemp.pos.x = assimpMesh->mVertices[i].x;
			vTemp.pos.y = assimpMesh->mVertices[i].y;
			vTemp.pos.z = assimpMesh->mVertices[i].z;
		}
		else
		{
			Log::PrintSeverity(Log::Severity::CRITICAL, "Mesh has no positions");
		}

		// Get Normals
		if (assimpMesh->HasNormals())
		{
			vTemp.normal.x = assimpMesh->mNormals[i].x;
			vTemp.normal.y = assimpMesh->mNormals[i].y;
			vTemp.normal.z = assimpMesh->mNormals[i].z;
		}
		else
		{
			Log::PrintSeverity(Log::Severity::CRITICAL, "Mesh has no normals");
		}

		if (assimpMesh->HasTangentsAndBitangents())
		{
			vTemp.tangent.x = assimpMesh->mTangents[i].x;
			vTemp.tangent.y = assimpMesh->mTangents[i].y;
			vTemp.tangent.z = assimpMesh->mTangents[i].z;
		}
		else
		{
			Log::PrintSeverity(Log::Severity::CRITICAL, "Mesh has no tangents");
		}
		
		
		// Get texture coordinates if there are any
		if (assimpMesh->HasTextureCoords(0))
		{
			vTemp.uv.x = (float)assimpMesh->mTextureCoords[0][i].x;
			vTemp.uv.y = (float)assimpMesh->mTextureCoords[0][i].y;
		}
		else
		{
			Log::PrintSeverity(Log::Severity::CRITICAL, "Mesh has no textureCoords");
		}

		vertices.push_back(vTemp);
	}

	// Get indices
	for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		aiFace face = assimpMesh->mFaces[i];
	
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create Mesh
	Mesh* mesh = new Mesh(
		m_pDevice,
		vertices, indices,
		m_pDescriptorHeap_CBV_UAV_SRV,
		*filePath);

	// ---------- Get Textures and set them to the m_pMesh START----------
	aiMaterial* mat = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
	
	// Split filepath
	std::string filePathWithoutTexture = *filePath;
	std::size_t indicesInPath = filePathWithoutTexture.find_last_of("/\\");
	filePathWithoutTexture = filePathWithoutTexture.substr(0, indicesInPath + 1);

	// Add the textures to the m_pMesh
	Texture* texture = nullptr;
	for (int i = 0; i < TEXTURE_TYPE::NUM_TEXTURE_TYPES; i++)
	{
		TEXTURE_TYPE type = static_cast<TEXTURE_TYPE>(i);
		texture = processTexture(mat, type, &filePathWithoutTexture);
		mesh->GetMaterial()->SetTexture(type, texture);
	}
	// ---------- Get Textures and set them to the m_pMesh END----------

	// Set shininess
	float shininess = 100;
	// Todo: looks to bright with these values, bad models or bad scene?
	// if (AI_SUCCESS != aiGetMaterialFloat(mat, AI_MATKEY_SHININESS, &shininess))
	// {
	// 	// if unsuccessful set a default
	// 	shininess = 20.0f;
	// }

	mesh->GetMaterial()->SetShininess(shininess);

	return mesh;
}

Texture* AssetLoader::processTexture(aiMaterial* mat,
	TEXTURE_TYPE texture_type,
	const std::string* filePathWithoutTexture)
{
	aiTextureType type;
	aiString str;
	Texture* texture = nullptr;

	// incase of the texture doesn't exist
	std::wstring defaultPath = L"";
	std::string warningMessageTextureType = "";

	// Find the textureType
	switch (texture_type)
	{
	case::TEXTURE_TYPE::AMBIENT:
		type = aiTextureType_AMBIENT;
		defaultPath = m_FilePathDefaultTextures + L"default_ambient.png";
		warningMessageTextureType = "Ambient";
		break;
	case::TEXTURE_TYPE::DIFFUSE:
		type = aiTextureType_DIFFUSE;
		defaultPath = m_FilePathDefaultTextures + L"default_diffuse.jpg";
		warningMessageTextureType = "Diffuse";
		break;
	case::TEXTURE_TYPE::SPECULAR:
		type = aiTextureType_SPECULAR;
		defaultPath = m_FilePathDefaultTextures + L"default_specular.png";
		warningMessageTextureType = "Specular";
		break;
	case::TEXTURE_TYPE::NORMAL:
		type = aiTextureType_NORMALS;
		defaultPath = m_FilePathDefaultTextures + L"default_normal.png";
		warningMessageTextureType = "Normal";
		break;
	case::TEXTURE_TYPE::EMISSIVE:
		type = aiTextureType_EMISSIVE;
		defaultPath = m_FilePathDefaultTextures + L"default_emissive.png";
		warningMessageTextureType = "Emissive";
		break;
	}

	mat->GetTexture(type, 0, &str);
	std::string textureFile = str.C_Str();
	if (textureFile.size() != 0)
	{
		texture = LoadTexture(to_wstring(*filePathWithoutTexture + textureFile).c_str());
	}

	if (texture != nullptr)
	{
		return texture;
	}
	else
	{
		// No texture, warn and apply default Texture
		Log::PrintSeverity(Log::Severity::WARNING, "Applying default texture: " + warningMessageTextureType + 
			" on mesh with path: \'%s\'\n", filePathWithoutTexture->c_str());
		return m_LoadedTextures[defaultPath].second;
	}

	return nullptr;
}
