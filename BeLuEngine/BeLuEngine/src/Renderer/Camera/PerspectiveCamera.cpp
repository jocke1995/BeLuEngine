#include "stdafx.h"
#include "PerspectiveCamera.h"

PerspectiveCamera::PerspectiveCamera(
	DirectX::XMVECTOR position, DirectX::XMVECTOR direction,
	double fov, double aspectRatio, double nearZ, double farZ,
	bool isPrimary)
	:BaseCamera(position, direction, isPrimary)
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

void PerspectiveCamera::RotateCamera(int yaw, int pitch)
{
	static const float rotationSpeed = 0.001f;
	m_Yaw += yaw * rotationSpeed;
	m_Pitch += pitch * rotationSpeed;
}

void PerspectiveCamera::UpdateMovement(int3 newMovement)
{
	m_MoveLeftRight += newMovement.x;
	m_MoveForwardBackward += newMovement.z;
	m_MoveUpDown += newMovement.y;
}

void PerspectiveCamera::updateProjectionMatrix()
{
	m_ProjMatrix = DirectX::XMMatrixPerspectiveFovLH(m_Fov, m_AspectRatio, m_NearZ, m_FarZ);
}

void PerspectiveCamera::updateSpecific(double dt)
{
	if (m_IsPrimary == true)
	{
		updateCameraMovement(dt);
	}

	m_ViewMatrix = DirectX::XMMatrixLookAtLH(m_EyeVector, DirectX::XMVectorAdd(m_EyeVector, m_DirectionVector), m_UpVector);

	m_ViewProjMatrix = m_ViewMatrix * m_ProjMatrix;
	m_ViewProjTranposedMatrix = DirectX::XMMatrixTranspose(m_ViewProjMatrix);
}

void PerspectiveCamera::updateCameraMovement(double dt)
{
	// Todo, use quaternions
	m_CamRotationMatrix = DirectX::XMMatrixRotationRollPitchYaw(m_Pitch, m_Yaw, 0.0f);

	// Calculate Direction Vector
	m_DirectionVector = DirectX::XMVector3TransformCoord(s_DefaultForwardVector, m_CamRotationMatrix);
	m_DirectionVector = DirectX::XMVector3Normalize(m_DirectionVector);

	// Calculate Right Vector
	m_RightVector = DirectX::XMVector3TransformCoord(s_DefaultRightVector, m_CamRotationMatrix);
	m_RightVector = DirectX::XMVector3Normalize(m_RightVector);

	// Calculate Up Vector
	m_UpVector = DirectX::XMVector3Cross(m_DirectionVector, m_RightVector);
	m_UpVector = DirectX::XMVector3Normalize(m_UpVector);

	static int ms = 150;
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*((m_MoveLeftRight		  * ms)	* dt, m_RightVector));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*((m_MoveForwardBackward * ms) * dt, m_DirectionVector));
	m_EyeVector = DirectX::XMVectorAdd(m_EyeVector, DirectX::operator*((m_MoveUpDown		  * ms)	* dt, s_DefaultUpVector));
}
