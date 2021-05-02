#ifndef BOTTOMLEVELACCELERATIONSTRUCTURE_H
#define BOTTOMLEVELACCELERATIONSTRUCTURE_H

#include "AccelerationStructure.h"
class BottomLevelAccelerationStructure : public AccelerationStructure
{
public:
    BottomLevelAccelerationStructure();
    virtual ~BottomLevelAccelerationStructure();

    void AddVertexBuffer(
        Resource* vertexBuffer, uint32_t vertexCount,
        Resource* indexBuffer , uint32_t indexCount);

    void Reset() override;
    void GenerateBuffers(ID3D12Device5* pDevice) override;
    void SetupAccelerationStructureForBuilding(ID3D12Device5* pDevice, bool update) override;

private:
    friend class TopLevelAccelerationStructure;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertexBuffers = {};
};

#endif
