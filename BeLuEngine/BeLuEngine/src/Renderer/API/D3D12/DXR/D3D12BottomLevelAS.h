#ifndef D3D12BOTTOMLEVELAS_H
#define D3D12BOTTOMLEVELAS_H

#include "../../IBottomLevelAS.h"

class D3D12BottomLevelAS : public IBottomLevelAS
{
public:
    D3D12BottomLevelAS();
    virtual ~D3D12BottomLevelAS();

    virtual void AddGeometry(
        IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
        IGraphicsBuffer* indexBuffer , uint32_t indexCount) override;

    virtual void Reset() override;
    virtual void GenerateBuffers() override;
    virtual void SetupAccelerationStructureForBuilding(bool update) override;

    IGraphicsBuffer* GetRayTracingResultBuffer() const;

private:
    friend class D3D12TopLevelAS;
    friend class D3D12GraphicsContext;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_GeometryBuffers = {};

    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
};

#endif
