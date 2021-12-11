#ifndef TOPLEVELACCELERATIONSTRUCTURE_H
#define TOPLEVELACCELERATIONSTRUCTURE_H

#include "AccelerationStructure.h"

class BottomLevelAccelerationStructure;

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

class TopLevelAccelerationStructure : public AccelerationStructure
{
public:
    TopLevelAccelerationStructure();
    virtual ~TopLevelAccelerationStructure();

    void AddInstance(
        BottomLevelAccelerationStructure* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int hitGroupIndex);

    void Reset() override;
    void GenerateBuffers() override;
    void SetupAccelerationStructureForBuilding(bool update) override;

private:
    friend class TopLevelRenderTask;

    unsigned int m_InstanceDescsSizeInBytes = 0;

    std::vector<Instance> m_Instances;

    bool m_IsBuilt = false;

    // Sets to 0 in Reset(), increments for each instance for unique IDs.
    unsigned int m_InstanceCounter = 0;
};

#endif
