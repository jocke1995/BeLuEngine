#ifndef TOPLEVELACCELERATIONSTRUCTURE_H
#define TOPLEVELACCELERATIONSTRUCTURE_H

#include "AccelerationStructure.h"

class Resource;

struct Instance
{
    Instance(
        Resource* BLAS,
        const DirectX::XMMATRIX& transform,
        unsigned int instanceID,
        unsigned int hitGroupIndex)
        :m_pBLAS(BLAS), m_Transform(transform), m_ID(instanceID), m_HitGroupIndex(hitGroupIndex)
    {}

    Resource* m_pBLAS = nullptr;
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
        Resource* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int instanceID,
        unsigned int hitGroupIndex);

    // Call to remove all instances currently in the TLAS
    void Reset() override;

    // Call when adding or removing geometry from the TLAS
    void GenerateBuffers(ID3D12Device5* pDevice) override;

    // Call every frame to update with world matrices
    void SetupAccelerationStructureForBuilding(ID3D12Device5* pDevice, bool update) override;

private:
    unsigned int m_InstanceDescsSizeInBytes = 0;
    Resource* m_pInstanceDesc = nullptr;

    std::vector<Instance> m_Instances;
};

#endif
