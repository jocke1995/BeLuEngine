#include "stdafx.h"
#include "BaseCamera.h"

#include "../Events/EventBus.h"

BaseCamera::BaseCamera(DirectX::XMVECTOR position, DirectX::XMVECTOR direction, bool isPrimary)
{
	// Create View Matrix
	m_EyeVector = position;
	m_DirectionVector = direction;
	m_UpVector = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_RightVector = DirectX::XMVector3Cross(m_UpVector, m_DirectionVector);

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_EyeVector, DirectX::XMVectorAdd(m_DirectionVector, m_EyeVector), m_UpVector);
	m_ViewMatrixInverse = DirectX::XMMatrixInverse(nullptr, m_ViewMatrix);

	m_IsPrimary = isPrimary;

	if (m_IsPrimary == true)
	{
		EventBus::GetInstance().Subscribe(this, &BaseCamera::toggleCameraLookaround);
	}
}

BaseCamera::~BaseCamera()
{
	if (m_IsPrimary == true)
	{
		EventBus::GetInstance().Unsubscribe(this, &BaseCamera::toggleCameraLookaround);
	}
}

void BaseCamera::Update(double dt)
{
	updateSpecific(dt);
}

void BaseCamera::SetPosition(float x, float y, float z)
{
	m_EyeVector = DirectX::XMVectorSet(x, y, z, 1.0);
}

void BaseCamera::SetDirection(float x, float y, float z)
{
	m_DirectionVector = DirectX::XMVectorSet(x, y, z, 0.0f);
	m_RightVector = DirectX::XMVector3Cross(m_UpVector, m_DirectionVector);

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_EyeVector, DirectX::XMVectorAdd(m_DirectionVector, m_EyeVector), m_UpVector);
}

DirectX::XMFLOAT3 BaseCamera::GetPosition() const
{
	DirectX::XMFLOAT3 DXfloat3;
	DirectX::XMStoreFloat3(&DXfloat3, m_EyeVector);

	return DXfloat3;
}

DirectX::XMFLOAT3 BaseCamera::GetDirection() const
{
	DirectX::XMFLOAT3 DXfloat3;
	DirectX::XMStoreFloat3(&DXfloat3, m_DirectionVector);

	return DXfloat3;
}

DirectX::XMFLOAT3 BaseCamera::GetUpVector() const
{
	DirectX::XMFLOAT3 DXfloat3;
	DirectX::XMStoreFloat3(&DXfloat3, m_UpVector);

	return DXfloat3;
}

DirectX::XMFLOAT3 BaseCamera::GetRightVector() const
{
	DirectX::XMFLOAT3 DXfloat3;
	DirectX::XMStoreFloat3(&DXfloat3, m_RightVector);

	return DXfloat3;
}

const float BaseCamera::GetNearPlane() const
{
	return 0.0f;
}

const float BaseCamera::GetFarPlane() const
{
	return 0.0f;
}

const DirectX::XMMATRIX* BaseCamera::GetViewMatrix() const
{
	return &m_ViewMatrix;
}

const DirectX::XMMATRIX* BaseCamera::GetViewMatrixInverse() const
{
	return &m_ViewMatrixInverse;
}

bool BaseCamera::IsCameraLookaroundEnabled() const
{
	return m_CameraLookaroundEnabled;
}

void BaseCamera::toggleCameraLookaround(ToggleCameraLookAround* event)
{
	m_CameraLookaroundEnabled = event->m_Enabled;
}
