#include "stdafx.h"
#include "ModelComponent.h"

#include "GPU_Structs.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Geometry/Model.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"
#include "../Renderer/Renderer.h"
#include "../Entity.h"

// TODO ABSTRACTION
#include "../Renderer/API/IGraphicsBuffer.h"

namespace component
{
	ModelComponent::ModelComponent(Entity* parent)
		:Component(parent)
	{
	}

	ModelComponent::~ModelComponent()
	{
		BL_SAFE_DELETE(m_SlotInfoByteAdressBuffer);
		BL_SAFE_DELETE(m_MaterialByteAdressBuffer);
	}

	void ModelComponent::SetModel(Model* model)
	{
		m_pModel = model;

		unsigned int numMeshes = m_pModel->GetSize();

		m_Materials.resize(numMeshes);
		m_MaterialDataRawBuffer.resize(numMeshes);
		m_SlotInfos.resize(numMeshes);

		// Start with values from the default material specified for that model
		for (unsigned int i = 0; i < numMeshes; i++)
			this->SetMaterialAt(i, m_pModel->GetOriginalMaterial()->at(i));

		updateSlotInfoBuffer();
		
		std::string fileName = std::filesystem::path(to_string(m_pModel->GetPath())).filename().string();
		m_SlotInfoByteAdressBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RawBuffer, sizeof(SlotInfo)	 , numMeshes, DXGI_FORMAT_UNKNOWN, to_wstring(fileName) + L"_RAWBUFFER_SLOTINFO");
		m_MaterialByteAdressBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RawBuffer, sizeof(MaterialData), numMeshes, DXGI_FORMAT_UNKNOWN, to_wstring(fileName) + L"_RAWBUFFER_MATERIALDATA");
	}

	void ModelComponent::SetDrawFlag(unsigned int drawFlag)
	{
		m_DrawFlag = drawFlag;
	}

	void ModelComponent::Update(double dt)
	{
	}

	void ModelComponent::OnInitScene()
	{
		Renderer::GetInstance().InitModelComponent(this);
	}

	void ModelComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitModelComponent(this);
	}

	Mesh* ModelComponent::GetMeshAt(unsigned int index) const
	{
		return m_pModel->GetMeshAt(index);
	}

	void ModelComponent::SetMaterialAt(unsigned int index, Material* material)
	{
		m_Materials[index] = material;

		// Need the materialData in a rawBuffer with proper stride to eachother for the rawByteAdressBuffer
		m_MaterialDataRawBuffer[index] =
		{
			material->GetSharedMaterialData()->textureAlbedo,
			material->GetSharedMaterialData()->textureRoughness,
			material->GetSharedMaterialData()->textureMetallic,
			material->GetSharedMaterialData()->textureNormal,

			material->GetSharedMaterialData()->textureEmissive,
			material->GetSharedMaterialData()->textureOpacity,
			material->GetSharedMaterialData()->hasEmissiveTexture,
			material->GetSharedMaterialData()->hasRoughnessTexture,

			material->GetSharedMaterialData()->hasMetallicTexture,
			material->GetSharedMaterialData()->hasOpacityTexture,
			material->GetSharedMaterialData()->hasNormalTexture,
			material->GetSharedMaterialData()->glow,

			material->GetSharedMaterialData()->emissiveValue,

			material->GetSharedMaterialData()->roughnessValue,
			material->GetSharedMaterialData()->metallicValue,
			material->GetSharedMaterialData()->opacityValue,
			material->GetSharedMaterialData()->pad3,
		};
	}

	Material* ModelComponent::GetMaterialAt(unsigned int index)
	{
		return m_Materials[index];
	}

	MaterialData* ModelComponent::GetUniqueMaterialDataAt(unsigned int index)
	{
		return &m_MaterialDataRawBuffer[index];
	}

	IGraphicsBuffer* ModelComponent::GetMaterialByteAdressBuffer() const
	{
		return m_MaterialByteAdressBuffer;
	}

	const SlotInfo* ModelComponent::GetSlotInfoAt(unsigned int index) const
	{
		return &m_SlotInfos[index];
	}

	IGraphicsBuffer* ModelComponent::GetSlotInfoByteAdressBufferDXR() const
	{
		return m_SlotInfoByteAdressBuffer;
	}

	void ModelComponent::updateSlotInfoBuffer()
	{
		for (unsigned int i = 0; i < m_pModel->GetSize(); i++)
		{
			m_SlotInfos[i] =
			{
				m_pModel->GetMeshAt(i)->GetVertexBuffer()->GetShaderResourceHeapIndex(),
				m_pModel->GetMeshAt(i)->GetIndexBuffer()->GetShaderResourceHeapIndex(),
				i,	// This will be used for raster, to move (sizeof(MaterialData) * i) in the raw structuredBuffer
				0 // Padding
			};
		}
	}

	void ModelComponent::UpdateMaterialRawBufferFromMaterial()
	{
		for (unsigned int i = 0; i < m_pModel->GetSize(); i++)
		{
			m_MaterialDataRawBuffer[i] =
			{
				m_Materials[i]->GetSharedMaterialData()->textureAlbedo,
				m_Materials[i]->GetSharedMaterialData()->textureRoughness,
				m_Materials[i]->GetSharedMaterialData()->textureMetallic,
				m_Materials[i]->GetSharedMaterialData()->textureNormal,

				m_Materials[i]->GetSharedMaterialData()->textureEmissive,
				m_Materials[i]->GetSharedMaterialData()->textureOpacity,
				m_Materials[i]->GetSharedMaterialData()->hasEmissiveTexture,
				m_Materials[i]->GetSharedMaterialData()->hasRoughnessTexture,

				m_Materials[i]->GetSharedMaterialData()->hasMetallicTexture,
				m_Materials[i]->GetSharedMaterialData()->hasOpacityTexture,
				m_Materials[i]->GetSharedMaterialData()->hasNormalTexture,
				m_Materials[i]->GetSharedMaterialData()->glow,

				m_Materials[i]->GetSharedMaterialData()->emissiveValue,

				m_Materials[i]->GetSharedMaterialData()->roughnessValue,
				m_Materials[i]->GetSharedMaterialData()->metallicValue,
				m_Materials[i]->GetSharedMaterialData()->opacityValue,
				m_Materials[i]->GetSharedMaterialData()->pad3,
			};
		}
	}

	unsigned int ModelComponent::GetDrawFlag() const
	{
		return m_DrawFlag;
	}

	unsigned int ModelComponent::GetNrOfMeshes() const
	{
		return m_pModel->GetSize();
	}

	const std::wstring& ModelComponent::GetModelPath() const
	{
		return m_pModel->GetPath();
	}

	bool ModelComponent::IsPickedThisFrame() const
	{
		return m_IsPickedThisFrame;
	}

	Model* ModelComponent::GetModel() const
	{
		return m_pModel;
	}


}
