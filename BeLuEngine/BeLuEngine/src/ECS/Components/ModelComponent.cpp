#include "stdafx.h"
#include "ModelComponent.h"

#include "GPU_Structs.h"
#include "../Renderer/Geometry/Model.h"
#include "../Renderer/Geometry/Mesh.h"
#include "../Renderer/Geometry/Material.h"
#include "../Renderer/Renderer.h"
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
		
	}

	void ModelComponent::SetModel(Model* model)
	{
		m_pModel = model;

		// Start with values from the default material specified for that model
		m_Materials = *m_pModel->GetOriginalMaterial();

		m_SlotInfos.resize(m_pModel->GetSize());
		updateSlotInfo();
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

	void ModelComponent::updateSlotInfo()
	{
		for (unsigned int i = 0; i < m_pModel->GetSize(); i++)
		{
			m_SlotInfos[i] =
			{
				m_pModel->GetMeshAt(i)->GetSRV()->GetDescriptorHeapIndex(),
				m_Materials[i]->GetMaterialData()->first->GetCBV()->GetDescriptorHeapIndex(),
				0, 0 // Padding
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
