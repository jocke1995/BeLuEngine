#ifndef D3D12GRAPHICSMANAGER_H
#define D3D12GRAPHICSMANAGER_H

#include "../Interface/IGraphicsManager.h"

// Native D3D12 Objects
struct ID3D12Device5;

class D3D12DescriptorHeap;
class D3D12GraphicsContext;
class IGraphicsTexture;

namespace component
{
	class ModelComponent;
}

struct DynamicDataParams
{
	ID3D12Resource* uploadResource = nullptr;
	unsigned __int64 offsetFromStart = 0;
	D3D12_GPU_VIRTUAL_ADDRESS vAddr = 0;
};

class D3D12GraphicsManager : public IGraphicsManager
{
public:
	D3D12GraphicsManager();
	virtual ~D3D12GraphicsManager();

	static D3D12GraphicsManager* GetInstance();

	void Init(HWND hwnd, unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat) override;

	// Call everyframe
	void Begin() override final;
	void ExecuteGraphicsContexts(const std::vector<IGraphicsContext*>& graphicsContexts, unsigned int numGraphicsContexts) override final;
	void SyncAndPresent(IGraphicsTexture* finalColorTexture) override final;
	void End() override final;

	static bool CHECK_HRESULT(HRESULT hrParam);

	void AddD3D12ObjectToDefferedDeletion(ID3D12Object* object);
	DynamicDataParams SetDynamicData(unsigned int size, const void* data);

	// Getters
	D3D12DescriptorHeap* GetMainDescriptorHeap() const;
	D3D12DescriptorHeap* GetRTVDescriptorHeap()  const;
	D3D12DescriptorHeap* GetDSVDescriptorHeap()  const;
	ID3D12Device5* GetDevice() const { return m_pDevice5; }
	ID3D12RootSignature* GetGlobalRootSignature() const { return m_pGlobalRootSig; }
	unsigned int GetCommandInterfaceIndex() const { return m_CommandInterfaceIndex; }
private:
	friend class ImGuiHandler;
	friend class RayTracingPipelineGenerator;	// ABSTRACTION TEMP
	// -------------------------- Native D3D12 -------------------------- 
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
	/*---------- Main Sync primitive -----------------*/
	HANDLE m_MainFenceEventHandle = nullptr;
	ID3D12Fence1* m_pMainFence = nullptr;
	UINT64 m_MainFenceValue = 0;

	/*---------- Secondary Sync Primitive for hardsyncs -----------------*/
	HANDLE m_HardSyncFenceEventHandle = nullptr;
	ID3D12Fence1* m_pHardSyncFence = nullptr;
	UINT64 m_HardSynceFenceValue = 0;

	// DescriptorHeaps
	D3D12DescriptorHeap* m_pMainDescriptorHeap = nullptr;
	D3D12DescriptorHeap* m_pRTVDescriptorHeap  = nullptr;
	D3D12DescriptorHeap* m_pDSVDescriptorHeap  = nullptr;

	// Swapchain
	IDXGISwapChain4* m_pSwapChain4 = nullptr;
	std::array<ID3D12Resource*, NUM_SWAP_BUFFERS> m_SwapchainResources;
	std::array<unsigned int, NUM_SWAP_BUFFERS> m_SwapchainRTVIndices;
	std::array<unsigned int, NUM_SWAP_BUFFERS> m_SwapchainSRVIndices;

	// CommandList used to copyResource at the end of the frame from the finalColor to the swapChain
	D3D12GraphicsContext* m_pGraphicsContext = nullptr;
	// Root Signature
	ID3D12RootSignature* m_pGlobalRootSig = nullptr;

	// Objects to be deffered deleted.
	// Pair information: <indexSubmitted, objectItSelf>
	std::vector<std::pair<unsigned int, ID3D12Object*>> m_ObjectsToBeDeleted;

	ID3D12Resource1* m_pIntermediateUploadHeap[NUM_SWAP_BUFFERS] = {};
	void* m_pIntermediateUploadHeapBegin[NUM_SWAP_BUFFERS] = {};
	LONG m_pIntermediateUploadHeapAtomicCurrentOffset = 0;
	const unsigned int m_IntermediateUploadHeapSize = 1024 * 1024 * 100; // 100MB
	// -------------------------- Native D3D12 -------------------------- 

	// -------------------------- Misc -------------------------- 
	unsigned int m_CommandInterfaceIndex = 0;
	unsigned int mFrameIndex = 0;

	// -------------------------- Private Functions -------------------------- 
	// Sync
	void waitForFrame(unsigned int frameToWaitFor);
	void waitForGPU(ID3D12CommandQueue* commandQueue);

	void deleteDefferedDeletionObjects(bool forceDeleteAll);
};

#endif