#include "stdafx.h"
#include "ModelComponent.h"

#include "GPU_Structs.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/Geometry/Model.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"
#include "../Renderer/Renderer.h"
#include "../Renderer/DescriptorHeap.h"
#include "../Entity.h"

#include "../Renderer/GPUMemory/GPUMemory.h"

namespace component
{
	ModelComponent::ModelComponent(Entity* parent)
		:Component(parent)
	{
	}

	ModelComponent::~ModelComponent()
	{
		delete m_SlotInfoByteAdressBuffer;
		delete m_MaterialByteAdressBuffer;
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

		D3D12_SHADER_RESOURCE_VIEW_DESC rawBufferDesc = {};
		rawBufferDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		rawBufferDesc.Buffer.FirstElement = 0;
		rawBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		rawBufferDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		rawBufferDesc.Buffer.NumElements = numMeshes;
		rawBufferDesc.Buffer.StructureByteStride = 0;
		rawBufferDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		
		Renderer& r = Renderer::GetInstance();
		std::string fileName = std::filesystem::path(to_string(m_pModel->GetPath())).filename().string();
		m_SlotInfoByteAdressBuffer = new ShaderResource(
			r.m_pDevice5, sizeof(SlotInfo) * numMeshes,
			to_wstring(fileName) + L"_RAWBUFFER_SLOTINFO",
			&rawBufferDesc,
			r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);

		m_MaterialByteAdressBuffer = new ShaderResource(
			r.m_pDevice5, sizeof(MaterialData) * numMeshes,
			to_wstring(fileName) + L"_RAWBUFFER_MATERIALDATA",
			&rawBufferDesc,
			r.m_DescriptorHeaps[E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV]);
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

	ShaderResource* ModelComponent::GetMaterialByteAdressBuffer() const
	{
		return m_MaterialByteAdressBuffer;
	}

	const SlotInfo* ModelComponent::GetSlotInfoAt(unsigned int index) const
	{
		return &m_SlotInfos[index];
	}

	ShaderResource* ModelComponent::GetSlotInfoByteAdressBufferDXR() const
	{
		return m_SlotInfoByteAdressBuffer;
	}

	void ModelComponent::updateSlotInfoBuffer()
	{
		for (unsigned int i = 0; i < m_pModel->GetSize(); i++)
		{
			m_SlotInfos[i] =
			{
				m_pModel->GetMeshAt(i)->GetVBSRV()->GetDescriptorHeapIndex(),
				m_pModel->GetMeshAt(i)->GetIBSRV()->GetDescriptorHeapIndex(),
				i,	// This will be used for raster, to move (sizeof(MaterialData) * i) in the buffer
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
