#ifndef ITOPLEVELAS_H
#define ITOPLEVELAS_H

class IBottomLevelAS;
class IGraphicsBuffer;

struct Instance
{
    Instance(
        IGraphicsBuffer* BLAS,
        const DirectX::XMMATRIX& transform,
        unsigned int instanceID,
        unsigned int hitGroupIndex,
        bool giveShadows)
        :m_pBLAS(BLAS), m_Transform(transform), m_ID(instanceID), m_HitGroupIndex(hitGroupIndex), m_GiveShadows(giveShadows)
    {}

    IGraphicsBuffer* m_pBLAS = nullptr;
    const DirectX::XMMATRIX& m_Transform = {};
    unsigned int m_ID = 0;
    unsigned int m_HitGroupIndex = 0;
    bool m_GiveShadows = true;
};

class ITopLevelAS
{
public:
    virtual ~ITopLevelAS();

    static ITopLevelAS* Create();

    virtual void AddInstance(
        IBottomLevelAS* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int hitGroupIndex,
        bool giveShadows) = 0;

    virtual void Reset() = 0;
    virtual void GenerateBuffers() = 0;
    virtual void SetupAccelerationStructureForBuilding(bool update) = 0;

    virtual IGraphicsBuffer* GetRayTracingResultBuffer() const = 0;

private:
};

#endif
