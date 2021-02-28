#include "stdafx.h"
#include "Light.h"

#include "../../Entity.h"

#include "../../../Renderer/Camera/OrthographicCamera.h"
#include "../../../Renderer/Camera/PerspectiveCamera.h"

Light::Light(unsigned int lightFlags)
{
	m_Id = s_LightIdCounter++;

	m_LightFlags = lightFlags;

	m_pBaseLight = new BaseLight();
	m_pBaseLight->color= { 1.0f, 1.0f, 1.0f };
	m_pBaseLight->castShadow = false;
}

Light::~Light()
{
	delete m_pBaseLight;
}

bool Light::operator==(const Light& other)
{
	return m_Id == other.m_Id;
}

void Light::SetColor(float3 color)
{
	m_pBaseLight->color = color;

	UpdateLightColor();
}

unsigned int Light::GetLightFlags() const
{
	return m_LightFlags;
}
