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

    void AddInstance(
        IBottomLevelAS* BLAS,
        const DirectX::XMMATRIX& m_Transform,
        unsigned int hitGroupIndex,
        bool giveShadows);
    void Reset();

    // Override these
    virtual bool CreateResultBuffer() = 0;
    virtual void MapResultBuffer() = 0;

    virtual IGraphicsBuffer* GetRayTracingResultBuffer() const = 0;

protected:
    std::vector<Instance> m_Instances;
    unsigned int m_InstanceCounter = 0; // Sets to 0 in Reset(), increments for each instance for unique IDs.
};

#endif
