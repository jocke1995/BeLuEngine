#ifndef ACCELERATIONSTRUCTURE_H
#define ACCELERATIONSTRUCTURE_H

class IGraphicsBuffer;

class AccelerationStructure
{
public:
    AccelerationStructure();
    virtual ~AccelerationStructure();

    // Call to flush the member variable of the AS to be able to create a new one from scratch
    virtual void Reset() = 0;

    // Call when adding or removing geometry from the AS
    virtual void GenerateBuffers() = 0;

    // Setup the buildDesc after each element is added in the AS
    virtual void SetupAccelerationStructureForBuilding(bool update) = 0;

    void BuildAccelerationStructure(ID3D12GraphicsCommandList4* commandList) const;

    IGraphicsBuffer* GetRayTracingResultBuffer() const;

protected:
    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
};

#endif
