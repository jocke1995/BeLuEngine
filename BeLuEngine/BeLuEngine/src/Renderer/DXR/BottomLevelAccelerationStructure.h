#ifndef BOTTOMLEVELACCELERATIONSTRUCTURE_H
#define BOTTOMLEVELACCELERATIONSTRUCTURE_H

#include "AccelerationStructure.h"
class BottomLevelAccelerationStructure : public AccelerationStructure
{
public:
    BottomLevelAccelerationStructure();
    virtual ~BottomLevelAccelerationStructure();

    void AddVertexBuffer(
        ID3D12Resource* vertexBuffer,
        uint32_t vertexCount,
        ID3D12Resource* indexBuffer,
        uint32_t indexCount,
        bool isOpaque = true);

    void FinalizeAccelerationStructure(ID3D12Device5* pDevice) override;

private:
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertexBuffers = {};
};

#endif
