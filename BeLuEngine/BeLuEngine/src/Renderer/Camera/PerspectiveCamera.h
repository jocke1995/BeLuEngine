#ifndef PERSPECTIVECAMERA_H
#define PERSPECTIVECAMERA_H

#include "BaseCamera.h"

class PerspectiveCamera : public BaseCamera
{
public:
    PerspectiveCamera(
        DirectX::XMVECTOR position,
        DirectX::XMVECTOR direction,
        float fov = 60.0f,
        float aspectRatio = 16.0f / 9.0f,
        float nearZ = 0.1f,
        float farZ = 3000.0f,
        bool isPrimary = false);

	virtual ~PerspectiveCamera();

    // Gets
    const DirectX::XMMATRIX* GetViewProjection() const;
    const DirectX::XMMATRIX* GetViewProjectionTranposed() const;

    // Sets on projection
    void SetFov(float fov);
    void SetAspectRatio(float aspectRatio);
    void SetNearZ(float nearPlaneDistance);
    void SetFarZ(float farPlaneDistance);

    // Will be called from inputEvent in inputComponent
    void RotateCamera(float yaw, float pitch);
    void UpdateMovement(float3 newMovement);

private:
    DirectX::XMMATRIX m_CamRotationMatrix;
    float m_MoveLeftRight = 0.0f;
    float m_MoveForwardBackward = 0.0f;
    float m_MoveUpDown = 0.0f;
    float m_Yaw = 0.0f;
    float m_Pitch = 0.0f;

    DirectX::XMMATRIX m_ProjMatrix;
    DirectX::XMMATRIX m_ViewProjMatrix;
    DirectX::XMMATRIX m_ViewProjTranposedMatrix;

    double m_Fov = 0.0f;
    double m_AspectRatio = 0.0f;
    double m_NearZ = 0.0f;
    double m_FarZ = 0.0f;

    void updateProjectionMatrix();
    void updateSpecific(double dt);
    void updateCameraMovement(double dt);
};

#endif