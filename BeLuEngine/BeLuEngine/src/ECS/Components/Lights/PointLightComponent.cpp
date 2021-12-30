#include "stdafx.h"
#include "PointLightComponent.h"

// Renderer
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Renderer.h"

// ECS
#include "../ECS/Entity.h"

namespace component
{
	PointLightComponent::PointLightComponent(Entity* parent, unsigned int lightFlags)
		:Component(parent), Light(lightFlags)
	{
		m_pPointLight = new PointLight();
		m_pPointLight->position = { 0.0f,  2.0f,  0.0f, 0.0f };
		m_pPointLight->baseLight = *m_pBaseLight;
	}

	PointLightComponent::~PointLightComponent()
	{
		delete m_pPointLight;
	}

	void PointLightComponent::Update(double dt)
	{
		if (m_LightFlags & static_cast<unsigned int>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION))
		{
			Transform* tc = m_pParent->GetComponent<TransformComponent>()->GetTransform();
			float3 position = tc->GetPositionFloat3();
			m_pPointLight->position.x = position.x;
			m_pPointLight->position.y = position.y;
			m_pPointLight->position.z = position.z;
		}

		// Copy into buffer to be updated on GPU
		// Pointlights are second in the buffer, first is dirlight, so we need to add the offset first.
		unsigned int offset = sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET;
		
		offset += m_LightOffsetInArray * sizeof(PointLight);

		// Copy lightData
		memcpy(Light::m_pRawData + offset * sizeof(unsigned char), m_pPointLight, sizeof(PointLight));
	}

	void PointLightComponent::OnInitScene()
	{
		Renderer::GetInstance().InitPointLightComponent(this);
	}

	void PointLightComponent::OnUnInitScene()
	{
		Renderer::GetInstance().UnInitPointLightComponent(this);
	}

	void PointLightComponent::SetPosition(float3 position)
	{
		m_pPointLight->position = { position.x, position.y, position.z, 1.0f };
	}

	void* PointLightComponent::GetLightData() const
	{
		return m_pPointLight;
	}

	void PointLightComponent::UpdateLightColor()
	{
		m_pPointLight->baseLight.color = m_pBaseLight->color;
	}

	void PointLightComponent::UpdateLightIntensity()
	{
		m_pPointLight->baseLight.intensity = m_pBaseLight->intensity;
	}
}