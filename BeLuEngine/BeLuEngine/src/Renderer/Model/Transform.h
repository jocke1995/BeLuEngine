#ifndef TRANSFORM_H
#define TRANSFORM_H

class Transform
{
public:
	Transform(bool invertDirection = false);
	virtual ~Transform();

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 pos);
	// Sets the movement direction. This will later be normalized to the velocity of the transform.
	
	void SetRotationX(float radians);
	void SetRotationY(float radians);
	void SetRotationZ(float radians);

	void SetScale(float scale);
	void SetScale(float x, float y, float z);
	void IncreaseScaleByPercent(float scale);

	void UpdateWorldMatrix();

	DirectX::XMMATRIX* GetWorldMatrix();
	DirectX::XMMATRIX* GetWorldMatrixTransposed();

	DirectX::XMFLOAT3 GetPositionXMFLOAT3() const;
	float3 GetPositionFloat3() const;

	DirectX::XMFLOAT3 GetScale() const;

	// gets a quaternion representation of the rotation matrix
	float4 GetRotation();
	// gets the rotation of the transform in all axisis
	DirectX::XMMATRIX GetRotMatrix() const;

	DirectX::XMFLOAT3 GetForwardXMFLOAT3() const;
	float3 GetForwardFloat3() const;

	DirectX::XMFLOAT3 GetRightXMFLOAT3() const;
	float3 GetRightFloat3() const;

	DirectX::XMFLOAT3 GetUpXMFLOAT3() const;
	float3 GetUpFloat3() const;
private:
	DirectX::XMMATRIX m_WorldMat;
	DirectX::XMMATRIX m_WorldMatTransposed;

	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Scale;

	DirectX::XMMATRIX m_RotXMat;
	DirectX::XMMATRIX m_RotYMat;
	DirectX::XMMATRIX m_RotZMat;
	DirectX::XMMATRIX m_RotationMat;
};

#endif