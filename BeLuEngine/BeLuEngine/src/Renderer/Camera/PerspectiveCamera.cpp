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

void PerspectiveCamera::MoveCamera(float3 direction)
{
	m_MoveLeftRight = direction.x;
	m_MoveUpDown = direction.y;
	m_MoveForwardBackward = direction.z;
}

void PerspectiveCamera::RotateCamera(float yaw, float pitch)
{
	static const float rotationSpeed = 0.001f;
	m_Yaw += yaw * rotationSpeed;
	m_Pitch += pitch * rotationSpeed;
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
	// Todo, use quaternions
	m_CamRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0.0f);
	m_DirectionVector = DirectX::XMVector3TransformCoord(s_DefaultForwardVector, m_CamRotationMatrix);
	m_DirectionVector = DirectX::XMVector3Normalize(m_DirectionVector);

	m_RightVector = DirectX::XMVector3TransformCoord(s_DefaultRightVector, m_CamRotationMatrix);
	m_DirectionVector = DirectX::XMVector3TransformCoord(s_DefaultForwardVector, m_CamRotationMatrix);
	m_UpVector = DirectX::XMVector3Cross(m_DirectionVector, m_RightVector);

	static unsigned int ms = 500;
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_MoveLeftRight * dt * ms, m_RightVector));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_MoveForwardBackward * dt * ms, m_DirectionVector));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*(m_MoveUpDown * dt * ms, s_DefaultUpVector));

	m_MoveLeftRight = m_MoveForwardBackward = m_MoveUpDown = 0.0f;

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_EyeVector, DirectX::XMVectorAdd(m_EyeVector, m_DirectionVector), m_UpVector);
}
