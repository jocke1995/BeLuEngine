#ifndef D3D12BOTTOMLEVELAS_H
#define D3D12BOTTOMLEVELAS_H

#include "../../Interface/RayTracing/IBottomLevelAS.h"

class D3D12BottomLevelAS : public IBottomLevelAS
{
public:
    D3D12BottomLevelAS();
    virtual ~D3D12BottomLevelAS();

    virtual void AddGeometry(
        IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
        IGraphicsBuffer* indexBuffer , uint32_t indexCount) override;

    virtual void GenerateBuffers() override;
    virtual void SetupAccelerationStructureForBuilding() override;

    IGraphicsBuffer* GetRayTracingResultBuffer() const;

private:
    friend class D3D12GraphicsContext;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_GeometryBuffers = {};

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
};

#endif
