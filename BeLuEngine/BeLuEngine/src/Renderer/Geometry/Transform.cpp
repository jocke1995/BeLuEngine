#include "stdafx.h"
#include "Transform.h"

#include "../Misc/Log.h"

Transform::Transform(bool invertDirection)
{
	m_Position = DirectX::XMFLOAT3(0.0, 0.0, 0.0);
	m_Rot = DirectX::XMFLOAT3(0.0, 0.0, 0.0);
	m_Scale = DirectX::XMFLOAT3(1.0, 1.0, 1.0);

	UpdateWorldMatrix();

	m_RotQuat = {};
}

Transform::~Transform()
{
}

void Transform::SetPosition(float x, float y, float z)
{
	m_Position = {x, y, z};
}

void Transform::SetPosition(DirectX::XMFLOAT3 pos)
{
	m_Position = { pos.x, pos.y, pos.z };
}

void Transform::SetRotationX(float radians)
{
	m_Rot.x = radians;

	DirectX::XMVECTOR rotVector = DirectX::XMQuaternionRotationRollPitchYaw(m_Rot.x, m_Rot.y, m_Rot.z);
	//rotVector = DirectX::XMQuaternionNormalize(rotVector);
	DirectX::XMStoreFloat4(&m_RotQuat, rotVector);
}

void Transform::SetRotationY(float radians)
{
	m_Rot.y = radians;

	DirectX::XMVECTOR rotVector = DirectX::XMQuaternionRotationRollPitchYaw(m_Rot.x, m_Rot.y, m_Rot.z);
	//rotVector = DirectX::XMQuaternionNormalize(rotVector);
	DirectX::XMStoreFloat4(&m_RotQuat, rotVector);
}

void Transform::SetRotationZ(float radians)
{
	m_Rot.z = radians;

	DirectX::XMVECTOR rotVector = DirectX::XMQuaternionRotationRollPitchYaw(m_Rot.x, m_Rot.y, m_Rot.z);
	//rotVector = DirectX::XMQuaternionNormalize(rotVector);
	DirectX::XMStoreFloat4(&m_RotQuat, rotVector);
}

void Transform::SetScale(float scale)
{
	m_Scale = DirectX::XMFLOAT3(scale, scale, scale);
}

void Transform::SetScale(float x, float y, float z)
{
	m_Scale = DirectX::XMFLOAT3(x, y, z);
}

void Transform::IncreaseScaleByPercent(float scale)
{
	m_Scale.x += m_Scale.x * scale;
	m_Scale.y += m_Scale.y * scale;
	m_Scale.z += m_Scale.z * scale;
}

void Transform::UpdateWorldMatrix()
{
	DirectX::XMMATRIX posMat = DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	DirectX::XMMATRIX sclMat = DirectX::XMMatrixScaling(m_Scale.x, m_Scale.y, m_Scale.z);

	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	m_WorldMat = sclMat * rotMat * posMat;

	// Update transposed world matrix
	m_WorldMatTransposed = DirectX::XMMatrixTranspose(m_WorldMat);
}

DirectX::XMMATRIX* Transform::GetWorldMatrix()
{
	return &m_WorldMat;
}

DirectX::XMMATRIX* Transform::GetWorldMatrixTransposed()
{
	return &m_WorldMatTransposed;
}

DirectX::XMFLOAT3 Transform::GetPositionXMFLOAT3() const
{
	return m_Position;
}

float3 Transform::GetPositionFloat3() const
{
	float3 pos = {};
	pos.x = m_Position.x;
	pos.y = m_Position.y;
	pos.z = m_Position.z;
	return pos;
}

DirectX::XMFLOAT3 Transform::GetScale() const
{
	return m_Scale;
}

DirectX::XMFLOAT3 Transform::GetRot() const
{
	return m_Rot;
}

DirectX::XMMATRIX Transform::GetRotMatrix() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);
	return rotMat;
}

DirectX::XMFLOAT3 Transform::GetForwardXMFLOAT3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 forward;
	DirectX::XMStoreFloat3(&forward, rotMat.r[2]);

	return forward;
}

float3 Transform::GetForwardFloat3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 forward;
	DirectX::XMStoreFloat3(&forward, rotMat.r[2]);

	return { forward.x, forward.y, forward.z };
}

DirectX::XMFLOAT3 Transform::GetRightXMFLOAT3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 right;
	DirectX::XMStoreFloat3(&right, rotMat.r[0]);

	return DirectX::XMFLOAT3( right.x, right.y, right.z);
}
float3 Transform::GetRightFloat3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 right;
	DirectX::XMStoreFloat3(&right, rotMat.r[0]);

	return {  right.x, right.y, right.z };
}
DirectX::XMFLOAT3 Transform::GetUpXMFLOAT3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 up;
	DirectX::XMStoreFloat3(&up, rotMat.r[1]);

	return up;
}
float3 Transform::GetUpFloat3() const
{
	DirectX::XMVECTOR rotVector = DirectX::XMLoadFloat4(&m_RotQuat);
	DirectX::XMMATRIX rotMat = DirectX::XMMatrixRotationQuaternion(rotVector);

	DirectX::XMFLOAT3 up;
	DirectX::XMStoreFloat3(&up, rotMat.r[1]);

	return { up.x, up.y, up.z };
}

IGraphicsBuffer* Transform::GetConstantBuffer()
{
	BL_ASSERT(m_pConstantBuffer);
	return m_pConstantBuffer;
}
