#ifndef TRANSFORM_H
#define TRANSFORM_H

class IGraphicsBuffer;

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

	// Called from transformComponent
	void UpdateWorldMatrix();

	// Called from Renderer
	void UpdateWorldWVP(const DirectX::XMMATRIX& viewProjMatTransposed);

	DirectX::XMMATRIX* GetWorldMatrix();
	DirectX::XMMATRIX* GetWorldMatrixTransposed();

	DirectX::XMFLOAT3 GetPositionXMFLOAT3() const;
	float3 GetPositionFloat3() const;

	DirectX::XMFLOAT3 GetScale() const;

	// gets the rotation of the transform in all axisis
	DirectX::XMFLOAT3 GetRot() const;

	DirectX::XMMATRIX GetRotMatrix() const;

	DirectX::XMFLOAT3 GetForwardXMFLOAT3() const;
	float3 GetForwardFloat3() const;

	DirectX::XMFLOAT3 GetRightXMFLOAT3() const;
	float3 GetRightFloat3() const;

	DirectX::XMFLOAT3 GetUpXMFLOAT3() const;
	float3 GetUpFloat3() const;

	IGraphicsBuffer* GetConstantBuffer();
	const MATRICES_PER_OBJECT_STRUCT* GetMatricesPerObjectData() const;
private:
	friend class Renderer;
	friend class OutliningRenderTask;
	friend class WireframeRenderTask;
	friend class TransparentRenderTask;
	friend class DepthRenderTask;
	friend class DXRReflectionTask;
	friend class DeferredGeometryRenderTask;
	friend class DeferredLightRenderTask;

	DirectX::XMMATRIX m_WorldMat;
	DirectX::XMMATRIX m_WorldMatTransposed;

	IGraphicsBuffer* m_pConstantBuffer = nullptr;
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Rot;
	DirectX::XMFLOAT3 m_Scale;

	DirectX::XMFLOAT4 m_RotQuat;

	// The data that is copied from CPU -> GPU
	MATRICES_PER_OBJECT_STRUCT matricesPerObject = {};
};

#endif