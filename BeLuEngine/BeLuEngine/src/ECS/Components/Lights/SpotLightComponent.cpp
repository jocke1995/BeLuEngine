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

        // Copy into buffer to be updated on GPU
        // Spotlights are third in the buffer, first is Dirlight, second is Pointlight so we need to add the offset first.
        unsigned int offset = sizeof(LightHeader) + DIR_LIGHT_MAXOFFSET + POINT_LIGHT_MAXOFFSET;

        offset += m_LightOffsetInArray * sizeof(SpotLight);

        // Copy lightData
        memcpy(Light::m_pRawData + offset * sizeof(unsigned char), m_pSpotLight, sizeof(SpotLight));
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

    void SpotLightComponent::UpdateLightIntensity()
    {
        m_pSpotLight->baseLight.intensity = m_pBaseLight->intensity;
    }
}
