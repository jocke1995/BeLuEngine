#ifndef BOTTOMLEVELACCELERATIONSTRUCTURE_H
#define BOTTOMLEVELACCELERATIONSTRUCTURE_H

#include "AccelerationStructure.h"

class IGraphicsBuffer;

class BottomLevelAccelerationStructure : public AccelerationStructure
{
public:
    BottomLevelAccelerationStructure();
    virtual ~BottomLevelAccelerationStructure();

    void AddVertexBuffer(
        IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
        IGraphicsBuffer* indexBuffer , uint32_t indexCount);

    void Reset() override;
    void GenerateBuffers() override;
    void SetupAccelerationStructureForBuilding(bool update) override;

private:
    friend class TopLevelAccelerationStructure;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertexBuffers = {};
};

#endif
