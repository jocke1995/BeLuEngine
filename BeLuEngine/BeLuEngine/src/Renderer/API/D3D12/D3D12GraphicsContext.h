#ifndef D3D12GRAPHICSCONTEXT_H
#define D3D12GRAPHICSCONTEXT_H

#include "../IGraphicsContext.h"

// temp
#include "../Renderer/CommandInterface.h"

namespace component
{
	class ModelComponent;
}

class D3D12GraphicsContext: public IGraphicsContext
{
public:
	D3D12GraphicsContext(const std::wstring& name);
	virtual ~D3D12GraphicsContext();

	virtual void Begin(bool isComputePipeline) override;
	virtual void End() override;

    TODO("Fix Interfaces for the parameters");
    virtual void SetPipelineState(ID3D12PipelineState* pso) override;
    virtual void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primTop) override;

    virtual void SetViewPort(unsigned int width, unsigned int height, float topLeftX = 0.0f, float topLeftY = 0.0f, float minDepth = 0.0f, float maxDepth = 1.0f) override;
    virtual void SetScizzorRect(unsigned int right, unsigned int bottom, float left = 0, unsigned int top = 0) override;

    virtual void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, float clearColor[4]) override;

    virtual void SetRenderTargets(  unsigned int numRenderTargets, D3D12_CPU_DESCRIPTOR_HANDLE* renderTargetDescriptors,
                                    bool RTsSingleHandleToDescriptorRange, D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilDescriptor) override;

    //virtual void SetConstantBufferView(unsigned int slot, Resource* resource, bool isComputePipeline) override;
    //virtual void SetShaderResourceView(unsigned int slot, Resource* resource, bool isComputePipeline) override;
    virtual void Set32BitConstant(unsigned int slot, unsigned int num32BitValuesToSet, unsigned int* pSrcData, unsigned int offsetIn32BitValues, bool isComputePipeline) override;

    virtual void SetIndexBuffer(D3D12_INDEX_BUFFER_VIEW* indexBufferView) override;

    virtual void DrawIndexedInstanced(unsigned int IndexCountPerInstance, unsigned int InstanceCount, unsigned int StartIndexLocation, int BaseVertexLocation, unsigned int StartInstanceLocation) override;

private:
	ID3D12GraphicsCommandList5* m_pCommandList{ nullptr };
	ID3D12CommandAllocator* m_pCommandAllocators[NUM_SWAP_BUFFERS]{ nullptr };

	// Useful for debugging
#ifdef DEBUG
	std::wstring m_Name = L"GraphicsContextDefault";
#endif
};

#endif