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
	}

	void ModelComponent::SetModel(Model* model)
	{
		m_pModel = model;

		// Start with values from the default material specified for that model
		m_Materials = *m_pModel->GetOriginalMaterial();

		m_SlotInfos.resize(m_pModel->GetSize());
		updateSlotInfo();

		unsigned int numMeshes = m_pModel->GetSize();

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

	Material* ModelComponent::GetMaterialAt(unsigned int index) const
	{
		return m_Materials[index];
	}

	void ModelComponent::SetMaterialAt(unsigned int index, Material* material)
	{
		m_Materials[index] = material;
		updateSlotInfo();
	}

	const SlotInfo* ModelComponent::GetSlotInfoAt(unsigned int index) const
	{
		return &m_SlotInfos[index];
	}

	ShaderResource* ModelComponent::GetByteAdressInfoDXR() const
	{
		return m_SlotInfoByteAdressBuffer;
	}

	void ModelComponent::updateSlotInfo()
	{
		for (unsigned int i = 0; i < m_pModel->GetSize(); i++)
		{
			m_SlotInfos[i] =
			{
				m_pModel->GetMeshAt(i)->GetVBSRV()->GetDescriptorHeapIndex(),
				m_pModel->GetMeshAt(i)->GetIBSRV()->GetDescriptorHeapIndex(),
				m_Materials[i]->GetMaterialData()->first->GetCBV()->GetDescriptorHeapIndex(),
				0, // Padding
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
