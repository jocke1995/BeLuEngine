#include "stdafx.h"
#include "SpotLightComponent.h"
#include "EngineMath.h"

// Renderer
#include "../Renderer/Geometry/Transform.h"
#include "../Renderer/Renderer.h"

// ECS
#include "../ECS/Entity.h"

namespace component
{
    SpotLightComponent::SpotLightComponent(Entity* parent, unsigned int lightFlags)
        :Component(parent), Light(lightFlags)
    {
        
        m_pSpotLight = new SpotLight();
        m_pSpotLight->position_cutOff = { 0.0f, 0.0f, 0.0f, cos(DirectX::XMConvertToRadians(30.0f)) };
        m_pSpotLight->direction_outerCutoff = { 1.0f, 0.0f, 0.0f, cos(DirectX::XMConvertToRadians(45.0f)) };
        m_pSpotLight->attenuation = { 1.0f, 0.027f, 0.0028f, 0.0f };
        m_pSpotLight->baseLight = *m_pBaseLight;

        m_pSpotLight->textureShadowMap = 0;

        initFlagUsages();
    }

    SpotLightComponent::~SpotLightComponent()
    {
        delete m_pSpotLight;
    }

    void SpotLightComponent::Update(double dt)
    {
        if (m_LightFlags & static_cast<unsigned int>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION))
        {
            Transform* tc = m_pParent->GetComponent<TransformComponent>()->GetTransform();
            float3 position = tc->GetPositionFloat3();
            m_pSpotLight->position_cutOff.x = position.x;
            m_pSpotLight->position_cutOff.y = position.y;
            m_pSpotLight->position_cutOff.z = position.z;
        }
    }

    void SpotLightComponent::OnInitScene()
    {
        this->Update(0);

        Renderer::GetInstance().InitSpotLightComponent(this);
    }

    void SpotLightComponent::OnUnInitScene()
    {
        Renderer::GetInstance().UnInitSpotLightComponent(this);
    }

    void SpotLightComponent::SetCutOff(float degrees)
    {
        m_pSpotLight->position_cutOff.w = cos(DirectX::XMConvertToRadians(degrees));
    }

    void SpotLightComponent::SetAttenuation(float3 attenuation)
    {
        m_pSpotLight->attenuation.x = attenuation.x;
        m_pSpotLight->attenuation.y = attenuation.y;
        m_pSpotLight->attenuation.z = attenuation.z;
    }

    // This function modifies the camera aswell as the position
    void SpotLightComponent::SetPosition(float3 position)
    {
        m_pSpotLight->position_cutOff.x = position.x;
        m_pSpotLight->position_cutOff.y = position.y;
        m_pSpotLight->position_cutOff.z = position.z;
    }

    // This function modifies the camera aswell as the direction
    void SpotLightComponent::SetDirection(float3 direction)
    {
        m_pSpotLight->direction_outerCutoff.x = direction.x;
        m_pSpotLight->direction_outerCutoff.y = direction.y;
        m_pSpotLight->direction_outerCutoff.z = direction.z;
    }

    // This function modifies the camera aswell as the outerCutOff
    void SpotLightComponent::SetOuterCutOff(float degrees)
    {
        float cameraFov = degrees * 2.0f;
        if (degrees >= 89)
        {
            degrees = 89.0f;
            cameraFov = 179.0f;
        }

        m_pSpotLight->direction_outerCutoff.w = cos(DirectX::XMConvertToRadians(degrees));
    }

    void* SpotLightComponent::GetLightData() const
    {
        return m_pSpotLight;
    }

    void SpotLightComponent::initFlagUsages()
    {
        if (m_LightFlags & static_cast<unsigned int>(F_LIGHT_FLAGS::USE_TRANSFORM_POSITION))
        {
            Transform* tc = m_pParent->GetComponent<TransformComponent>()->GetTransform();
            float3 position = tc->GetPositionFloat3();
            m_pSpotLight->position_cutOff.x = position.x;
            m_pSpotLight->position_cutOff.y = position.y;
            m_pSpotLight->position_cutOff.z = position.z;
        }

        if (m_LightFlags & static_cast<unsigned int>(F_LIGHT_FLAGS::CAST_SHADOW))
        {
            m_pSpotLight->baseLight.castShadow = true;
        }
    }

    void SpotLightComponent::UpdateLightColor()
    {
        m_pSpotLight->baseLight.color = m_pBaseLight->color;
    }
}
