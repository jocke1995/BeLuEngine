#include "stdafx.h"
#include "SkyboxComponent.h"

#include "../ECS/Entity.h"
#include "../Renderer/Renderer.h"

#include "../Renderer/API/Interface/IGraphicsBuffer.h"

namespace component
{
	SkyboxComponent::SkyboxComponent(Entity* parent)
		:Component(parent)
	{
	}

	SkyboxComponent::~SkyboxComponent()
	{
	}

	void SkyboxComponent::Update(double dt)
	{
	}

	void SkyboxComponent::OnInitScene()
	{
		Renderer::GetInstance().InitSkyboxComponent(this);
	}

	void SkyboxComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitSkyboxComponent(this);
	}

	void SkyboxComponent::Reset()
	{
	}

	void SkyboxComponent::SetSkyboxTexture(IGraphicsTexture* texture)
	{
		m_pSkyBoxTexture = texture;
	}

	IGraphicsTexture* SkyboxComponent::GetSkyboxTexture() const
	{
		BL_ASSERT(m_pSkyBoxTexture);
		return m_pSkyBoxTexture;
	}
}
