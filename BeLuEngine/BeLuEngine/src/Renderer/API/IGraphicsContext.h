#ifndef IGRAPHICSCONTEXT_H
#define IGRAPHICSCONTEXT_H

class IGraphicsContext
{
public:
    IGraphicsContext();
    virtual ~IGraphicsContext();

    virtual void Begin() = 0;
    virtual void SetupBindings(bool isComputePipeline) = 0;
    virtual void End() = 0;

    TODO("Fix Interfaces for the parameters");
    virtual void SetPipelineState(ID3D12PipelineState* pso) = 0;
    virtual void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop) = 0;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) = 0;

    virtual void ClearDepthTexture(IGraphicsTexture* depthTexture, float depthValue, bool clearStencil, unsigned int stencilValue) = 0;
    virtual void ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4]) = 0;
    virtual void SetRenderTargets(unsigned int numRenderTargets, IGraphicsTexture* renderTargetTextures[], IGraphicsTexture* depthTexture) = 0;

    /* ---------------------- Use E_COMMON_DESCRIPTOR_BINDINGS as first param if possible ---------------------- */
    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsTexture* graphicsTexture, bool isComputePipeline) = 0;
    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) = 0;
    virtual void SetConstantBuffer(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) = 0;
    virtual void Set32BitConstants(unsigned int rootParamSlot, unsigned int num32BitValuesToSet, const void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) = 0;
    /* ---------------------- Use E_COMMON_DESCRIPTOR_BINDINGS as first param if possible ---------------------- */

    virtual void SetIndexBuffer(IGraphicsBuffer* indexBuffer, unsigned int sizeInBytes) = 0;

    virtual void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int StartIndexLocation, int BaseVertexLocation, unsigned int StartInstanceLocation) = 0;


private:
};

#endif
