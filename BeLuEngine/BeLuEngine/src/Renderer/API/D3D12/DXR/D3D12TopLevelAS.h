#ifndef D3D12TOPLEVELAS_H
#define D3D12TOPLEVELAS_H

#include "../../Interface/RayTracing/ITopLevelAS.h"

class D3D12TopLevelAS : public ITopLevelAS
{
public:
    D3D12TopLevelAS();
    virtual ~D3D12TopLevelAS();

    // To be called by mainthread before Rendering scene
    // Generates a new result buffer if needed.
    // Returns true if it created a new buffer, otherwise false
    virtual bool CreateResultBuffer() override final;

    // To be called inside the TLAS renderTask
    // Puts the actual data inside the resultBuffer
    virtual void MapResultBuffer() override final;

    virtual IGraphicsBuffer* GetRayTracingResultBuffer() const override final;

private:
    friend class D3D12GraphicsContext;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
    unsigned int m_InstanceDescsSizeInBytes = 0;

    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;
    unsigned int m_ResultBufferMaxNumberOfInstances = 0;

    bool reAllocateInstanceDescBuffers(unsigned int newSizeInBytes);
    IGraphicsBuffer* m_pInstanceDescBuffers[NUM_SWAP_BUFFERS] = {};
    unsigned int m_CurrentMaxInstanceDescSize = 0;
};

#endif
