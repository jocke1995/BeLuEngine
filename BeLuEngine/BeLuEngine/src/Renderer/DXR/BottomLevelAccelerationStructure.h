#ifndef BOTTOMLEVELACCELERATIONSTRUCTURE_H
#define BOTTOMLEVELACCELERATIONSTRUCTURE_H

class IGraphicsBuffer;

class BottomLevelAccelerationStructure
{
public:
    BottomLevelAccelerationStructure();
    virtual ~BottomLevelAccelerationStructure();

    void AddVertexBuffer(
        IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
        IGraphicsBuffer* indexBuffer , uint32_t indexCount);

    void Reset();
    void GenerateBuffers();
    void SetupAccelerationStructureForBuilding(bool update);

    IGraphicsBuffer* GetRayTracingResultBuffer() const;
    const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& GetBuildDesc() const;

private:
    friend class TopLevelAccelerationStructure;

	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_vertexBuffers = {};

    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC m_BuildDesc = {};
};

#endif
