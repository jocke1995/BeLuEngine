#ifndef D3D12GRAPHICSMANAGER_H
#define D3D12GRAPHICSMANAGER_H

#include "../IGraphicsManager.h"

// Native D3D12 Objects
struct ID3D12Device5;

class D3D12GraphicsManager : public IGraphicsManager
{
public:
	D3D12GraphicsManager();
	virtual ~D3D12GraphicsManager();

	void Init() override;
private:
	// -------------------------- Native D3D12 Objects -------------------------- 
	ID3D12Device5* m_pDevice5 = nullptr;

	// Needed for updating VRAM info in ImGui
	IDXGIAdapter4* m_pAdapter4 = nullptr;
	HANDLE m_ProcessHandle = nullptr;

	// Queues
	ID3D12CommandQueue* m_pGraphicsCommandQueue = nullptr;
	ID3D12CommandQueue* m_pComputeCommandQueue = nullptr;
	ID3D12CommandQueue* m_pCopyCommandQueue = nullptr;
	// -------------------------- Native D3D12 Objects -------------------------- 
};

#endif