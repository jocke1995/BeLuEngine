#ifndef D3D12GRAPHICSCONTEXT_H
#define D3D12GRAPHICSCONTEXT_H

#include "../IGraphicsContext.h"

namespace component
{
	class ModelComponent;
}

class IGraphicsTexture;
class IGraphicsBuffer;

struct ImDrawData;

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
    virtual void ResourceBarrier(IGraphicsTexture* graphicsTexture, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) override final;
    virtual void ResourceBarrier(IGraphicsBuffer* graphicsBuffer, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) override final;
    virtual void UAVBarrier(IGraphicsTexture* graphicsTexture) override final;
    virtual void UAVBarrier(IGraphicsBuffer* graphicsBuffer) override final;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) override final;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) override final;

    virtual void ClearDepthTexture(IGraphicsTexture* depthTexture, bool clearDepth, float depthValue, bool clearStencil, unsigned int stencilValue) override final;
    virtual void ClearRenderTarget(IGraphicsTexture* renderTargetTexture, float clearColor[4]) override final;
    virtual void ClearUAVTextureFloat(IGraphicsTexture* uavTexture, float clearValues[4]) override final;
    virtual void ClearUAVTextureUINT(IGraphicsTexture* uavTexture, unsigned int clearValues[4]) override final;
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

    TODO("Abstract")
    // Raytracing
    virtual void BuildAccelerationStructure(const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc) override final;
    virtual void DispatchRays(const D3D12_DISPATCH_RAYS_DESC& dispatchRaysDesc) override final;
    virtual void SetPipelineState(ID3D12StateObject* pso) override final;

private:
    friend class D3D12GraphicsManager;
    friend class ScopedPIXEvent;

    // TEMP
    friend class ImGuiRenderTask;
    friend class TonemapComputeTask;

	ID3D12GraphicsCommandList5* m_pCommandList{ nullptr };
	ID3D12CommandAllocator* m_pCommandAllocators[NUM_SWAP_BUFFERS]{ nullptr };

	// Useful for debugging
#ifdef DEBUG
	std::wstring m_Name = L"GraphicsContextDefault";
#endif
};

#endif