#ifndef D3D12GRAPHICSMANAGER_H
#define D3D12GRAPHICSMANAGER_H

#include "../IGraphicsManager.h"

// Native D3D12 Objects
struct ID3D12Device5;

// D3D12 Wrappers
class DescriptorHeap;

namespace component
{
	class ModelComponent;
}

class D3D12GraphicsManager : public IGraphicsManager
{
public:
	D3D12GraphicsManager();
	virtual ~D3D12GraphicsManager();

	void Init() override;

	// Getters
	DescriptorHeap* GetMainDescriptorHeap() const;
	DescriptorHeap* GetRTVDescriptorHeap()  const;
	DescriptorHeap* GetDSVDescriptorHeap()  const;
private:
	// ABSTRACTION TEMP
	friend class Renderer;
	friend class component::ModelComponent;
	friend class ImGuiHandler;
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

	// CommandInterface
	std::vector<ID3D12CommandList*> m_DirectCommandLists[NUM_SWAP_BUFFERS];
	std::vector<ID3D12CommandList*> m_ImGuiCommandLists[NUM_SWAP_BUFFERS];

	// DescriptorHeaps
	DescriptorHeap* m_pMainDescriptorHeap = nullptr;
	DescriptorHeap* m_pRTVDescriptorHeap  = nullptr;
	DescriptorHeap* m_pDSVDescriptorHeap  = nullptr;
	// -------------------------- Native D3D12 Objects -------------------------- 
};

#endif