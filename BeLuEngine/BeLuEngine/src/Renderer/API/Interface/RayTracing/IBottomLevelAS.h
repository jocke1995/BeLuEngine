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

    virtual void Reset() = 0;
    virtual void GenerateBuffers() = 0;
    virtual void SetupAccelerationStructureForBuilding(bool update) = 0;

    virtual IGraphicsBuffer* GetRayTracingResultBuffer() const = 0;

private:
};

#endif
