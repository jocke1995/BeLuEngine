#ifndef IBOTTOMLEVELAS_H
#define IBOTTOMLEVELAS_H

class IGraphicsBuffer;

class IBottomLevelAS
{
public:
    virtual ~IBottomLevelAS();

    static IBottomLevelAS* Create();

    virtual void AddGeometry(
        IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
        IGraphicsBuffer* indexBuffer, uint32_t indexCount) = 0;

    virtual void GenerateBuffers() = 0;
    virtual void SetupAccelerationStructureForBuilding() = 0;

    virtual IGraphicsBuffer* GetRayTracingResultBuffer() const = 0;

protected:
    friend class D3D12TopLevelAS;

    IGraphicsBuffer* m_pScratchBuffer = nullptr;
    IGraphicsBuffer* m_pResultBuffer = nullptr;
private:
};

#endif
