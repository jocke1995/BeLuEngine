#ifndef D3D12GRAPHICSCONTEXT_H
#define D3D12GRAPHICSCONTEXT_H

#include "../Interface/IGraphicsContext.h"

#include "D3D12ResourceStateTracker.h"

struct PendingTransitionBarrier
{
    D3D12LocalStateTracker* localStateTracker = nullptr;
    unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    D3D12_RESOURCE_STATES afterState = (D3D12_RESOURCE_STATES)-1;
};

class D3D12GraphicsContext: public IGraphicsContext
{
public:
	D3D12GraphicsContext(const std::wstring& name);
	virtual ~D3D12GraphicsContext();

	virtual void Begin() override final;
	virtual void SetupBindings(bool isComputePipeline) override final;
	virtual void End() override final;

    TODO("Revisit this function");
    void UploadTexture(IGraphicsTexture* graphicsTexture) override final;
    void UploadBuffer(IGraphicsBuffer* graphicsBuffer, const void* data) override final;

    virtual void SetPipelineState(IGraphicsPipelineState* pso) override final;

    TODO("Fix Interfaces for the parameters");
    virtual void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop) override final;

    TODO("Fix Interfaces for the parameters");
    virtual void ResourceBarrier(IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES desiredState, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) override final;
    virtual void ResourceBarrier(IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES desiredState, unsigned int subResource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) override final;
    virtual void UAVBarrier(IGraphicsTexture* graphicsTexture) override final;
    virtual void UAVBarrier(IGraphicsBuffer* graphicsBuffer) override final;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) override final;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) override final;

    virtual void CopyTextureRegion(IGraphicsTexture* dst, IGraphicsTexture* src, unsigned int xStart, unsigned int yStart) override final;

    virtual void ClearDepthTexture(IGraphicsTexture* depthTexture, bool clearDepth, float depthValue, bool clearStencil, unsigned int stencilValue) override final;
    virtual void ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4]) override final;
    virtual void ClearUAVTextureFloat(IGraphicsTexture* uavTexture, float clearValues[4], unsigned int mipLevel = 0) override final;
    virtual void ClearUAVTextureUINT(IGraphicsTexture* uavTexture, unsigned int clearValues[4], unsigned int mipLevel = 0) override final;
    virtual void SetRenderTargets(unsigned int numRenderTargets, IGraphicsTexture* renderTargetTextures[], IGraphicsTexture* depthTexture) override final;

    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsTexture* graphicsTexture, bool isComputePipeline) override final;
    virtual void SetShaderResourceView(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) override final;
    virtual void SetConstantBuffer(unsigned int rootParamSlot, IGraphicsBuffer* graphicsBuffer, bool isComputePipeline) override final;
    virtual void SetConstantBufferDynamicData(unsigned int rootParamSlot, unsigned int size, const void* data, bool isComputePipeline) override final;
    virtual void Set32BitConstants(unsigned int rootParamSlot, unsigned int num32BitValuesToSet, const void* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) override final;

    virtual void SetStencilRef(unsigned int stencilRef) override final;

    virtual void SetIndexBuffer(IGraphicsBuffer* indexBuffer, unsigned int sizeInBytes) override final;

    virtual void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int StartIndexLocation, int BaseVertexLocation, unsigned int StartInstanceLocation) override final;

    virtual void Dispatch(unsigned int threadGroupsX, unsigned int threadGroupsY, unsigned int threadGroupsZ) override final;

    virtual void DrawImGui() override final;

    // Raytracing
    virtual void BuildTLAS(ITopLevelAS* pTlas) override;
    virtual void BuildBLAS(IBottomLevelAS* pBlas) override;
    virtual void DispatchRays(IShaderBindingTable* sbt, unsigned int dispatchWidth, unsigned int dispatchHeight, unsigned int dispatchDepth = 1) override final;
    virtual void SetRayTracingPipelineState(IRayTracingPipelineState* rtPipelineState) override final;

private:
    friend class D3D12GraphicsManager;
    friend class D3D12LocalStateTracker;
    friend class ScopedPIXEvent;

	ID3D12GraphicsCommandList5* m_pCommandList{ nullptr };
	ID3D12CommandAllocator* m_pCommandAllocators[NUM_SWAP_BUFFERS]{ nullptr };

    /* ---------------------------------- Automatic ResourceBarrier Management ----------------------------------------------- */
    ID3D12GraphicsCommandList5* m_pTransitionCommandList{ nullptr };
    ID3D12CommandAllocator* m_pTransitionCommandAllocators[NUM_SWAP_BUFFERS]{ nullptr };

    // Each local state can have multiple pending barriers, 1 for each subResource in the worst case scenario
    std::vector<PendingTransitionBarrier> m_PendingResourceBarriers = {};

    // For easy mapping between the unique globalStateTracker and the per-context localStateTracker
    std::unordered_map<D3D12GlobalStateTracker*, D3D12LocalStateTracker*> m_GlobalToLocalMap = {};

    // This function has to be called on the mainThread AFTER all renderPasses have been recorded
    // Returns false if no transitionBarriers were needed, in that case, we can skip to execute it
    bool resolvePendingTransitionBarriers();
    /* ---------------------------------- Automatic ResourceBarrier Management ----------------------------------------------- */

	// Useful for debugging
#ifdef DEBUG
	std::wstring m_Name = L"GraphicsContextDefault";
#endif
};

#endif