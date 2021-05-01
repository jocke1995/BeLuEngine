#ifndef ACCELERATIONSTRUCTURE_H
#define ACCELERATIONSTRUCTURE_H

class Resource;

class AccelerationStructure
{
public:
    AccelerationStructure();
    virtual ~AccelerationStructure();

    // Setup the buildDesc after each element is added in the AS
    virtual void FinalizeAccelerationStructure(ID3D12Device5* pDevice) = 0;

    void BuildAccelerationStructure(ID3D12GraphicsCommandList4* commandList) const;
protected:
    Resource* m_pScratch = nullptr;
    Resource* m_pResult = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
};

#endif
