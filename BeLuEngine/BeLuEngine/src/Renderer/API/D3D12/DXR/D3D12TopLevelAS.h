#ifndef D3D12TOPLEVELAS_H
#define D3D12TOPLEVELAS_H

#include "../../Interface/RayTracing/ITopLevelAS.h"

class D3D12TopLevelAS : public ITopLevelAS
{
public:
    D3D12TopLevelAS();
    virtual ~D3D12TopLevelAS();

    void AddInstance(
        IBottomLevelAS* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int hitGroupIndex);

    void Reset();
    void GenerateBuffers();
    void SetupAccelerationStructureForBuilding(bool update);

    IGraphicsBuffer* GetRayTracingResultBuffer() const;

private:
    friend class D3D12GraphicsContext;
    friend class TopLevelRenderTask;

    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
    unsigned int m_InstanceDescsSizeInBytes = 0;

    std::vector<Instance> m_Instances;
    unsigned int m_InstanceCounter = 0; // Sets to 0 in Reset(), increments for each instance for unique IDs.

    bool m_IsBuilt = false;
};

#endif
