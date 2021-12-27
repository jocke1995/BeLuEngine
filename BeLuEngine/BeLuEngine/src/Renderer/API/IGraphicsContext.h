#ifndef IGRAPHICSCONTEXT_H
#define IGRAPHICSCONTEXT_H

class IGraphicsTexture;
class IGraphicsBuffer;
class IGraphicsPipelineState;

class IGraphicsContext
{
public:
    IGraphicsContext();
    virtual ~IGraphicsContext();

    virtual void Begin() = 0;
    virtual void SetupBindings(bool isComputePipeline) = 0;
    virtual void End() = 0;

    virtual void UploadTexture(IGraphicsTexture* graphicsTexture) = 0;
    virtual void UploadBuffer(IGraphicsBuffer* graphicsBuffer, const void* data) = 0;

    virtual void SetPipelineState(IGraphicsPipelineState* pso) = 0;

    TODO("Fix Interfaces for the parameters");
    virtual void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop) = 0;

    TODO("Fix Interfaces for the parameters");
    virtual void ResourceBarrier(IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) = 0;
    virtual void ResourceBarrier(IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) = 0;
    virtual void UAVBarrier(IGraphicsTexture* graphicsTexture) = 0;
    virtual void UAVBarrier(IGraphicsBuffer* graphicsBuffer) = 0;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) = 0;

    virtual void ClearDepthTexture(IGraphicsTexture* depthTexture, bool clearDepth, float depthValue, bool clearStencil, unsigned int stencilValue) = 0;
    virtual void ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4]) = 0;
    virtual void SetRenderTargets(unsigned int numRenderTargets, IGraphicsTexture* renderTargetTextures[], IGraphicsTexture* depthTexture) = 0;

    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsTexture* graphicsTexture, bool isComputePipeline) = 0;
    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) = 0;
    virtual void SetConstantBuffer(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) = 0;
    virtual void SetConstantBufferDynamicData(unsigned int rootParamSlot, unsigned int size, const void* data, bool isComputePipeline) = 0;
    virtual void Set32BitConstants(unsigned int rootParamSlot, unsigned int num32BitValuesToSet, const void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) = 0;

    virtual void SetStencilRef(unsigned int stencilRef) = 0;

    virtual void SetIndexBuffer(IGraphicsBuffer* indexBuffer, unsigned int sizeInBytes) = 0;

    virtual void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int StartIndexLocation, int BaseVertexLocation, unsigned int StartInstanceLocation) = 0;

    virtual void Dispatch(unsigned int threadGroupsX, unsigned int threadGroupsY, unsigned int threadGroupsZ) = 0;

    virtual void DrawImGui() = 0;

    TODO("Wrapper");
    // Raytracing
    virtual void BuildAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc) = 0;
    virtual void DispatchRays(const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc) = 0;
    virtual void SetPipelineState(ID3D12StateObject* pso) = 0;

private:
};

#endif
