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

private:
	ID3D12GraphicsCommandList5* m_pCommandList{ nullptr };
	ID3D12CommandAllocator* m_pCommandAllocators[NUM_SWAP_BUFFERS]{ nullptr };

	// Useful for debugging
#ifdef DEBUG
	std::wstring m_Name = L"GraphicsContextDefault";
#endif
};

#endif