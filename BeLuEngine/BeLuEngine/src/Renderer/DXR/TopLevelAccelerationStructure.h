#ifndef TOPLEVELACCELERATIONSTRUCTURE_H
#define TOPLEVELACCELERATIONSTRUCTURE_H

class BottomLevelAccelerationStructure;
class IGraphicsBuffer;

struct Instance
{
    Instance(
        IGraphicsBuffer* BLAS,
        const DirectX::XMMATRIX& transform,
        unsigned int instanceID,
        unsigned int hitGroupIndex)
        :m_pBLAS(BLAS), m_Transform(transform), m_ID(instanceID), m_HitGroupIndex(hitGroupIndex)
    {}

    IGraphicsBuffer* m_pBLAS = nullptr;
    const DirectX::XMMATRIX& m_Transform = {};
    unsigned int m_ID = 0;
    unsigned int m_HitGroupIndex = 0;
};

class TopLevelAccelerationStructure
{
public:
    TopLevelAccelerationStructure();
    virtual ~TopLevelAccelerationStructure();

    void AddInstance(
        BottomLevelAccelerationStructure* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int hitGroupIndex);

    void Reset();
    void GenerateBuffers();
    void SetupAccelerationStructureForBuilding(bool update);

    IGraphicsBuffer* GetRayTracingResultBuffer() const;
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& GetBuildDesc() const;

private:
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
