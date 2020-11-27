#include "stdafx.h"
#include "PerspectiveCamera.h"

PerspectiveCamera::PerspectiveCamera(
	DirectX::XMVECTOR position, DirectX::XMVECTOR direction,
	float fov, float aspectRatio, float nearZ, float farZ)
	:BaseCamera(position, direction)
{
	m_Fov = fov * DirectX::XM_PI / 180.0f;
	m_AspectRatio = aspectRatio;
	m_NearZ = nearZ;
	m_FarZ = farZ;

	updateProjectionMatrix();

	// Init
	updateSpecific(0);
}

PerspectiveCamera::~PerspectiveCamera()
{

}

const DirectX::XMMATRIX* PerspectiveCamera::GetViewProjection() const
{
	return &m_ViewProjMatrix;
}

const DirectX::XMMATRIX* PerspectiveCamera::GetViewProjectionTranposed() const
{
	return &m_ViewProjTranposedMatrix;
}

void PerspectiveCamera::SetFov(float fov)
{
	m_Fov = fov * DirectX::XM_PI / 180.0f;
	updateProjectionMatrix();
}

void PerspectiveCamera::SetAspectRatio(float aspectRatio)
{
	m_AspectRatio = aspectRatio;
	updateProjectionMatrix();
}

void PerspectiveCamera::SetNearZ(float nearPlaneDistance)
{
	m_NearZ = nearPlaneDistance;
	updateProjectionMatrix();
}

void PerspectiveCamera::SetFarZ(float farPlaneDistance)
{
	m_FarZ = farPlaneDistance;
	updateProjectionMatrix();
}

void PerspectiveCamera::SetYaw(const float yaw)
{
	m_Yaw = yaw;
}

void PerspectiveCamera::SetPitch(const float pitch)
{
	m_Pitch = pitch;
}

const float PerspectiveCamera::GetYaw() const
{
	return m_Yaw;
}

const float PerspectiveCamera::GetPitch() const
{
	return m_Pitch;
}

void PerspectiveCamera::UpdateMovement(float x, float y, float z)
{
	m_MoveLeftRight += x;
	m_MoveForwardBackward += z;
	m_MoveUpDown += y;
}

void PerspectiveCamera::updateProjectionMatrix()
{
	m_ProjMatrix = DirectX::XMMatrixPerspectiveFovLH(m_Fov, m_AspectRatio, m_NearZ, m_FarZ);
}

void PerspectiveCamera::updateSpecific(double dt)
{
	updateCameraMovement(dt);

	m_ViewProjMatrix = m_ViewMatrix * m_ProjMatrix;
	m_ViewProjTranposedMatrix = DirectX::XMMatrixTranspose(m_ViewProjMatrix);
}

void PerspectiveCamera::updateCameraMovement(double dt)
{
	m_RightVector = DirectX::XMVector3Cross(m_UpVector, m_DirectionVector);
	
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_RightVector, m_MoveLeftRight * 10 * dt));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_DirectionVector,  m_MoveForwardBackward * 10 * dt));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_UpVector, m_MoveUpDown * 10 * dt));

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_EyeVector, DirectX::XMVectorAdd(m_EyeVector, m_DirectionVector), m_UpVector);
}
