#ifndef D3D12GRAPHICSMANAGER_H
#define D3D12GRAPHICSMANAGER_H

#include "../IGraphicsManager.h"

// Native D3D12 Objects
struct ID3D12Device5;

// D3D12 Wrappers
class DescriptorHeap;

// TEMP
#include "../Renderer/GPUMemory/GPUMemory.h"

namespace component
{
	class ModelComponent;
}

class D3D12GraphicsManager : public IGraphicsManager
{
public:
	D3D12GraphicsManager();
	virtual ~D3D12GraphicsManager();

	void Init(HWND hwnd, unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat) override;
	void Execute(const std::vector<ID3D12CommandList*>& m_DirectCommandLists, unsigned int numCommandLists); // This will later take in GPUContext and be overriding from base
	void Present() override;

	// Getters
	DescriptorHeap* GetMainDescriptorHeap() const;
	DescriptorHeap* GetRTVDescriptorHeap()  const;
	DescriptorHeap* GetDSVDescriptorHeap()  const;
private:
	// ABSTRACTION TEMP
	friend class Renderer;
	friend class component::ModelComponent;
	friend class ImGuiHandler;
	friend class ImGuiRenderTask;
	friend class MergeRenderTask;
	// -------------------------- Native D3D12 Objects -------------------------- 
	// D3D12 DLL
	HINSTANCE m_D3D12Handle;
	ID3D12Device5* m_pDevice5 = nullptr;

	// Needed for updating VRAM info in ImGui
	IDXGIAdapter4* m_pAdapter4 = nullptr;
	HANDLE m_ProcessHandle = nullptr;

	// Queues
	ID3D12CommandQueue* m_pGraphicsCommandQueue = nullptr;
	ID3D12CommandQueue* m_pComputeCommandQueue = nullptr;
	ID3D12CommandQueue* m_pCopyCommandQueue = nullptr;

	// Fences
	HANDLE m_EventHandle = nullptr;
	ID3D12Fence1* m_pFenceFrame = nullptr;
	UINT64 m_FenceFrameValue = 0;

	// DescriptorHeaps
	DescriptorHeap* m_pMainDescriptorHeap = nullptr;
	DescriptorHeap* m_pRTVDescriptorHeap  = nullptr;
	DescriptorHeap* m_pDSVDescriptorHeap  = nullptr;

	// Swapchain
	IDXGISwapChain4* m_pSwapChain4 = nullptr;
	std::array<Resource*, NUM_SWAP_BUFFERS> m_Resources;
	std::array<RenderTargetView*, NUM_SWAP_BUFFERS> m_RTVs;
	std::array<ShaderResourceView*, NUM_SWAP_BUFFERS> m_SRVs;
	// -------------------------- Native D3D12 Objects -------------------------- 

	void waitForGPU(ID3D12CommandQueue* commandQueue);
};

#endif