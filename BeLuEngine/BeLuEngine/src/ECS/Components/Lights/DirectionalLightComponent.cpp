#include "stdafx.h"
#include "DirectionalLightComponent.h"
#include "../Renderer/Renderer.h"

namespace component
{
	DirectionalLightComponent::DirectionalLightComponent(Entity* parent, unsigned int lightFlags)
		:Component(parent), Light(lightFlags)
	{
		m_pDirectionalLight = new DirectionalLight();
		m_pDirectionalLight->direction = { -1.0f,  -0.5f,  0.0f, 0.0f };
		m_pDirectionalLight->baseLight = *m_pBaseLight;

		m_pDirectionalLight->textureShadowMap = 0;

		initFlagUsages();
	}

	DirectionalLightComponent::~DirectionalLightComponent()
	{
		delete m_pDirectionalLight;
	}

	void DirectionalLightComponent::Update(double dt)
	{

	}

	void DirectionalLightComponent::OnInitScene()
	{
		this->Update(0);
		Renderer::GetInstance().InitDirectionalLightComponent(this);
	}

	void DirectionalLightComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitDirectionalLightComponent(this);
	}

	void DirectionalLightComponent::SetDirection(float3 direction)
	{
		m_pDirectionalLight->direction = { direction.x, direction.y, direction.z, 0.0f };
	}

	void* DirectionalLightComponent::GetLightData() const
	{
		return m_pDirectionalLight;
	}

	void DirectionalLightComponent::UpdateLightColor()
	{
		m_pDirectionalLight->baseLight.color = m_pBaseLight->color;
	}

	void DirectionalLightComponent::initFlagUsages()
	{
		if (m_LightFlags & static_cast<unsigned int>(F_LIGHT_FLAGS::CAST_SHADOW))
		{
			m_pDirectionalLight->baseLight.castShadow = true;
		}
	}
}
