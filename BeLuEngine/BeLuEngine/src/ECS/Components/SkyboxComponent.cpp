#include "stdafx.h"

#include "../ECS/Entity.h"
#include "SkyboxComponent.h"

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
	}

	void SkyboxComponent::OnUnInitScene()
	{
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
