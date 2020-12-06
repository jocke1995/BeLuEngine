#ifndef BASECAMERA_H
#define BASECAMERA_H

class BaseCamera
{
public:
	BaseCamera(
		DirectX::XMVECTOR position = { 0.0, 4.0, -10.0, 1.0f },
		DirectX::XMVECTOR direction = { 0.0f, -2.0f, 10.0f, 1.0f },
		bool isPrimary = false);
	virtual ~BaseCamera();

	void Update(double dt);

	void SetPosition(float x, float y, float z);
	void SetDirection(float x, float y, float z);

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT3 GetDirection() const;
	DirectX::XMFLOAT3 GetUpVector() const;
	DirectX::XMFLOAT3 GetRightVector() const;

	// Matrices
	const DirectX::XMMATRIX* GetViewMatrix() const;
	const DirectX::XMMATRIX* GetViewMatrixInverse() const;
	virtual const DirectX::XMMATRIX* GetViewProjection() const = 0;
	virtual const DirectX::XMMATRIX* GetViewProjectionTranposed() const = 0;

protected:
	DirectX::XMVECTOR m_RightVector;
	DirectX::XMVECTOR m_EyeVector;
	DirectX::XMVECTOR m_DirectionVector;
	DirectX::XMVECTOR m_UpVector;

	inline static const DirectX::XMVECTOR s_DefaultRightVector	 = { 1.0f, 0.0f, 0.0f, 0.0f };
	inline static const DirectX::XMVECTOR s_DefaultUpVector		 = { 0.0f, 1.0f, 0.0f, 0.0f };
	inline static const DirectX::XMVECTOR s_DefaultForwardVector = { 0.0f, 0.0f, 1.0f, 0.0f };

	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ViewMatrixInverse;

	bool m_IsPrimary = false;
	virtual void updateSpecific(double dt = 0.0) = 0;
};

#endif
