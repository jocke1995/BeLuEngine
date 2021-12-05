#ifndef IGRAPHICSCONTEXT_H
#define IGRAPHICSCONTEXT_H

TODO("Put this is a more fitting place");
enum class CommonDescriptorBindings
{
    // ConstantBuffers (B0... B4)
    SlotInfo = 0,
    DescriptorHeapIndices = 1,
    MATRICES_PER_OBJECT_STRUCT = 2,
    CB_PER_FRAME_STRUCT = 3,
    CB_PER_SCENE_STRUCT = 4,

    // SRVs (T0... T1)
    RawBufferLights = 0,
    GlobalRawBufferMaterial = 1
};

class IGraphicsContext
{
public:
    IGraphicsContext();
    virtual ~IGraphicsContext();

    virtual void Begin(bool isComputePipeline) = 0;
    virtual void End() = 0;

    TODO("Fix Interfaces for the parameters");
    virtual void SetPipelineState(ID3D12PipelineState* pso) = 0;
    virtual void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop) = 0;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) = 0;

    virtual void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, float clearColor[4]) = 0;

    virtual void SetRenderTargets(  unsigned int numRenderTargets, D3D12_CPU_DESCRIPTOR_HANDLE renderTargetDescriptors[],
                                    bool RTsSingleHandleToDescriptorRange, D3D12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor) = 0;

    virtual void SetConstantBufferView(CommonDescriptorBindings slot, Resource* resource, bool isComputePipeline) = 0;
    virtual void SetConstantBufferView(unsigned int slot, Resource* resource, bool isComputePipeline) = 0;
    virtual void SetShaderResourceView(CommonDescriptorBindings slot, Resource* resource, bool isComputePipeline) = 0;
    virtual void SetShaderResourceView(unsigned int slot, Resource* resource, bool isComputePipeline) = 0;

    virtual void Set32BitConstant(CommonDescriptorBindings slot, unsigned int num32BitValuesToSet, void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) = 0;
    virtual void Set32BitConstant(unsigned int slot, unsigned int num32BitValuesToSet, void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) = 0;

    virtual void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView) = 0;

    virtual void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int StartIndexLocation, int BaseVertexLocation, unsigned int StartInstanceLocation) = 0;


private:
};

#endif
