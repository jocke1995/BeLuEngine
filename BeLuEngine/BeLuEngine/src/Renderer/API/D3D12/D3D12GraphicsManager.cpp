#include "stdafx.h"
#include "D3D12GraphicsManager.h"

TODO("This should be inside stdafx.h");
#include "../Misc/Log.h"

#include "../Renderer/Statistics/EngineStatistics.h"
#include "../Renderer/DescriptorHeap.h"
D3D12GraphicsManager::D3D12GraphicsManager()
{
}

D3D12GraphicsManager::~D3D12GraphicsManager()
{
	if (!FreeLibrary(m_D3D12Handle))
	{
		BL_LOG_WARNING("Failed to Free D3D12 Handle\n");
	}

	BL_SAFE_RELEASE(&m_pDevice5);
	BL_SAFE_RELEASE(&m_pAdapter4);

	BL_SAFE_RELEASE(&m_pGraphicsCommandQueue);
	BL_SAFE_RELEASE(&m_pCopyCommandQueue);
	BL_SAFE_RELEASE(&m_pComputeCommandQueue);

	BL_SAFE_DELETE(m_pMainDescriptorHeap);
	BL_SAFE_DELETE(m_pRTVDescriptorHeap);
	BL_SAFE_DELETE(m_pDSVDescriptorHeap);

	BL_SAFE_RELEASE(&m_pSwapChain4);

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_DELETE(m_Resources[i]);
		BL_SAFE_DELETE(m_RTVs[i]);
		BL_SAFE_DELETE(m_SRVs[i]);
	}
}

void D3D12GraphicsManager::Init(HWND hwnd, unsigned int width, unsigned int height, DXGI_FORMAT dxgiFormat)
{
	HRESULT hr;
#pragma region CreateDevice
	bool deviceCreated = false;

#ifdef DEBUG && !defined(USE_NSIGHT_AFTERMATH)

	//Enable the D3D12 debug layer.
	ID3D12Debug3* debugController = nullptr;
	m_D3D12Handle = LoadLibrary(L"D3D12.dll");

	PFN_D3D12_GET_DEBUG_INTERFACE f = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(m_D3D12Handle, "D3D12GetDebugInterface");

	hr = f(IID_PPV_ARGS(&debugController));
	if (SUCCEEDED(hr))
	{
		EngineStatistics::GetIM_CommonStats().m_DebugLayerActive = true;

		if (ENABLE_DEBUGLAYER || ENABLE_VALIDATIONGLAYER)
		{
			debugController->EnableDebugLayer();

			if (ENABLE_VALIDATIONGLAYER)
			{
				debugController->SetEnableGPUBasedValidation(true);
			}

			IDXGIInfoQueue* dxgiInfoQueue = nullptr;
			unsigned int dxgiFactoryFlags = 0;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
				dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

				// Break on severity
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
				dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

				// Break on errors
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_UNKNOWN, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_MISCELLANEOUS, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_INITIALIZATION, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_CLEANUP, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_COMPILATION, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_CREATION, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_SETTING, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_STATE_GETTING, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_RESOURCE_MANIPULATION, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_EXECUTION, true);
				dxgiInfoQueue->SetBreakOnCategory(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_CATEGORY_SHADER, true);
			}

			BL_SAFE_RELEASE(&debugController);
		}
	}

#endif

	IDXGIFactory6* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));

	if (SUCCEEDED(hr))
	{
		for (unsigned int adapterIndex = 0;; ++adapterIndex)
		{
			adapter = nullptr;
			if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
			{
				break; // No more adapters
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			ID3D12Device5* pDevice = nullptr;
			HRESULT hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pDevice));

			if (SUCCEEDED(hr))
			{
				DXGI_ADAPTER_DESC adapterDesc = {};
				adapter->GetDesc(&adapterDesc);
				EngineStatistics::GetIM_CommonStats().m_Adapter = to_string(std::wstring(adapterDesc.Description));
				EngineStatistics::GetIM_CommonStats().m_API = "DirectX 12";

				D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
				hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
				BL_SAFE_RELEASE(&pDevice);

				if (SUCCEEDED(hr))
				{
					if (features5.RaytracingTier == D3D12_RAYTRACING_TIER_1_1)
					{
						BL_LOG_INFO("Raytracing tier 1.1 supported!\n");
						break; // found one!
					}
				}
			}

			BL_SAFE_RELEASE(&adapter);
		}
	}
	

	if (FAILED(adapter->QueryInterface(__uuidof(IDXGIAdapter4), reinterpret_cast<void**>(&m_pAdapter4))))
	{
		BL_LOG_CRITICAL("Failed to queryInterface for adapter4\n");
	}

	if (adapter)
	{
#if defined(USE_NSIGHT_AFTERMATH)
		m_GpuCrashTracker.Initialize();
#endif

		HRESULT hr = S_OK;
		//Create the actual device.
		if (SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice5))))
		{
			deviceCreated = true;

			// Init aftermathflags
#if defined(USE_NSIGHT_AFTERMATH)
			const uint32_t aftermathFlags =
				GFSDK_Aftermath_FeatureFlags_EnableMarkers |             // Enable event marker tracking.
				GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |    // Enable tracking of resources.
				GFSDK_Aftermath_FeatureFlags_CallStackCapturing |        // Capture call stacks for all draw calls, compute dispatches, and resource copies.
				GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;    // Generate debug information for shaders.

			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_Initialize(
				GFSDK_Aftermath_Version_API,
				aftermathFlags,
				m_pDevice5));
#endif
		}
		else
		{
			BL_LOG_CRITICAL("Failed to create Device\n");
		}

		BL_SAFE_RELEASE(&adapter);
	}
	else
	{
		BL_LOG_INFO("Did not find GPU, using WARP device");
		//Create warp m_pDevice if no adapter was found.
		factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pDevice5));
	}

	BL_SAFE_RELEASE(&factory);
#pragma endregion

#pragma region CreateCommandQueues
	// Direct
	D3D12_COMMAND_QUEUE_DESC cqdDirect = {};
	cqdDirect.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = m_pDevice5->CreateCommandQueue(&cqdDirect, IID_PPV_ARGS(&m_pGraphicsCommandQueue));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Direct CommandQueue\n");
	}
	m_pGraphicsCommandQueue->SetName(L"DirectQueue");

	// Compute
	D3D12_COMMAND_QUEUE_DESC cqdCompute = {};
	cqdCompute.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = m_pDevice5->CreateCommandQueue(&cqdCompute, IID_PPV_ARGS(&m_pComputeCommandQueue));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Compute CommandQueue\n");
	}
	m_pComputeCommandQueue->SetName(L"ComputeQueue");

	// Copy
	D3D12_COMMAND_QUEUE_DESC cqdCopy = {};
	cqdCopy.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_pDevice5->CreateCommandQueue(&cqdCopy, IID_PPV_ARGS(&m_pCopyCommandQueue));
	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Copy CommandQueue\n");
	}
	m_pComputeCommandQueue->SetName(L"CopyQueue");
#pragma endregion

#pragma region Fences
	hr = m_pDevice5->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFenceFrame));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Fence\n");
	}
	m_FenceFrameValue = 1;

	// Event handle to use for GPU Sync
	m_EventHandle = CreateEvent(0, false, false, L"EventHandle");
#pragma endregion

#pragma region DescriptorHeap
	m_pMainDescriptorHeap	 = new DescriptorHeap(m_pDevice5, E_DESCRIPTOR_HEAP_TYPE::CBV_UAV_SRV);
	m_pRTVDescriptorHeap	 = new DescriptorHeap(m_pDevice5, E_DESCRIPTOR_HEAP_TYPE::RTV);
	m_pDSVDescriptorHeap	 = new DescriptorHeap(m_pDevice5, E_DESCRIPTOR_HEAP_TYPE::DSV);
#pragma endregion

#pragma region Swapchain

	IDXGIFactory4* factorySwapChain = nullptr;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factorySwapChain));

	if (hr != S_OK)
	{
		BL_LOG_CRITICAL("Failed to create DXGIFactory for SwapChain\n");
	}

	//HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	//MONITORINFO mi = { sizeof(mi) };
	//GetMonitorInfo(hmon, &mi);
	//
	//int m_ScreenWidth = mi.rcMonitor.right - mi.rcMonitor.left;
	//int m_ScreenHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;

	//Create descriptor
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = width;
	scDesc.Height = height;
	scDesc.Format = dxgiFormat;
	scDesc.Stereo = FALSE;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = NUM_SWAP_BUFFERS;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Scaling = DXGI_SCALING_STRETCH;
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;


	IDXGISwapChain1* swapChain1 = nullptr;
	hr = factorySwapChain->CreateSwapChainForHwnd(
		m_pGraphicsCommandQueue,
		hwnd,
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1);

	if (SUCCEEDED(hr))
	{
		if (SUCCEEDED(swapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain4))))
		{
			m_pSwapChain4->Release();
		}
	}
	else
	{
		BL_LOG_CRITICAL("Failed to create Swapchain\n");
	}

	BL_SAFE_RELEASE(&factorySwapChain);

	// Set RTVs
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_Resources[i] = new Resource();
		m_RTVs[i] = new RenderTargetView(m_pDevice5, width, height, m_pRTVDescriptorHeap, nullptr, m_Resources[i], false);
	}

	// Connect the m_RenderTargets to the swapchain, so that the swapchain can easily swap between these two m_RenderTargets
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		HRESULT hr = m_pSwapChain4->GetBuffer(i, IID_PPV_ARGS(const_cast<Resource*>(m_RTVs[i]->GetResource())->GetID3D12Resource1PP()));
		if (FAILED(hr))
		{
			BL_LOG_CRITICAL("Failed to GetBuffer from RenderTarget to Swapchain\n");
		}

		m_RTVs[i]->CreateRTV(m_pDevice5, m_pRTVDescriptorHeap, nullptr);
	}

	m_RTVs[0]->GetResource()->GetID3D12Resource1()->SetName(L"RTV0_Resource");
	m_RTVs[1]->GetResource()->GetID3D12Resource1()->SetName(L"RTV1_Resource");

	// Create SRVs
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = dxgiFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_SRVs[i] = new ShaderResourceView(m_pDevice5, m_pMainDescriptorHeap, &srvDesc, m_Resources[i]);
	}

#pragma endregion
}

void D3D12GraphicsManager::Execute(const std::vector<ID3D12CommandList*>& m_DirectCommandLists, unsigned int numCommandLists)
{
	m_pGraphicsCommandQueue->ExecuteCommandLists(numCommandLists, m_DirectCommandLists.data());
}

void D3D12GraphicsManager::Present()
{
	// Hardsync atm
	waitForGPU(m_pGraphicsCommandQueue);

	HRESULT hr = m_pSwapChain4->Present(0, 0);

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
}

DescriptorHeap* D3D12GraphicsManager::GetMainDescriptorHeap() const
{
	return m_pMainDescriptorHeap;
}

DescriptorHeap* D3D12GraphicsManager::GetRTVDescriptorHeap() const
{
	return m_pRTVDescriptorHeap;
}

DescriptorHeap* D3D12GraphicsManager::GetDSVDescriptorHeap() const
{
	return m_pDSVDescriptorHeap;
}

void D3D12GraphicsManager::waitForGPU(ID3D12CommandQueue* commandQueue)
{
	static const unsigned int nrOfFenceChangesPerFrame = 1;
	static const unsigned int framesToBeAhead = 0;

	unsigned int fenceValuesToBeAhead = framesToBeAhead * nrOfFenceChangesPerFrame;

	m_pGraphicsCommandQueue->Signal(m_pFenceFrame, m_FenceFrameValue);
	//Wait if the CPU is "framesToBeAhead" number of frames ahead of the GPU
	if (m_pFenceFrame->GetCompletedValue() < m_FenceFrameValue - fenceValuesToBeAhead)
	{
		m_pFenceFrame->SetEventOnCompletion(m_FenceFrameValue - fenceValuesToBeAhead, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
	m_FenceFrameValue++;
}
