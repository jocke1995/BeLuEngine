#include "stdafx.h"
#include "D3D12GraphicsManager.h"

#include "../Misc/EngineStatistics.h"

#include "../Renderer/RenderPasses/Graphics/GraphicsPass.h"

#include "D3D12GraphicsContext.h"
#include "D3D12GraphicsTexture.h"
#include "D3D12DescriptorHeap.h"

D3D12GraphicsManager::D3D12GraphicsManager()
{
}

D3D12GraphicsManager::~D3D12GraphicsManager()
{
	// Make sure GPU is idle
	waitForGPU(m_pCopyCommandQueue);
	waitForGPU(m_pComputeCommandQueue);
	waitForGPU(m_pGraphicsCommandQueue);

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
	BL_SAFE_DELETE(m_pGraphicsContext);

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_RELEASE(&m_SwapchainResources[i]);
	}

	BL_SAFE_RELEASE(&m_pGlobalRootSig);

	// Intermediate Upload Heap
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_RELEASE(&m_pIntermediateUploadHeap[i]);
	}

	// Fences
	BL_ASSERT(CloseHandle(m_MainFenceEventHandle));
	BL_ASSERT(CloseHandle(m_HardSyncFenceEventHandle));
	BL_SAFE_RELEASE(&m_pMainFence);
	BL_SAFE_RELEASE(&m_pHardSyncFence);

	// Force defferedDelete on all objects
	deleteDefferedDeletionObjects(true);
}

D3D12GraphicsManager* D3D12GraphicsManager::GetInstance()
{
	IGraphicsManager* graphicsManager = IGraphicsManager::GetBaseInstance();

	return static_cast<D3D12GraphicsManager*>(graphicsManager);
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
		if (ENABLE_DEBUGLAYER || ENABLE_VALIDATIONGLAYER)
		{
			EngineStatistics::GetIM_CommonStats().m_DebugLayerActive = true;

			debugController->EnableDebugLayer();
			debugController->SetEnableGPUBasedValidation(ENABLE_VALIDATIONGLAYER);

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
	// Main Fence
	hr = m_pDevice5->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pMainFence));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Fence\n");
	}
	m_MainFenceValue = 1;

	// Event handle to use for GPU Sync
	m_MainFenceEventHandle = CreateEvent(0, false, false, L"EventHandle");

	// Hardsync fence
	hr = m_pDevice5->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pHardSyncFence));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to Create Fence\n");
	}
	m_HardSynceFenceValue = 0;

	// Event handle to use for GPU Sync
	m_HardSyncFenceEventHandle = CreateEvent(0, false, false, L"HardSyncEventHandle");
#pragma endregion

#pragma region DescriptorHeap
	m_pMainDescriptorHeap	 = new D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 10000, L"Main_CBV_UAV_SRV_DescriptorHeap", true);
	m_pRTVDescriptorHeap	 = new D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 50, L"Main_RTV_DescriptorHeap", false);
	m_pDSVDescriptorHeap	 = new D3D12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5, L"Main_DSV_DescriptorHeap", false);
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

	// Connect the m_RenderTargets to the swapchain, so that the swapchain can easily swap between these two m_RenderTargets
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		HRESULT hr = m_pSwapChain4->GetBuffer(i, IID_PPV_ARGS(&m_SwapchainResources[i]));
		if (!CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to GetBuffer from RenderTarget to Swapchain\n");
		}

		std::wstring swapchainName = L"SwapchainResource_" + std::to_wstring(i);
		hr = m_SwapchainResources[i]->SetName(swapchainName.c_str());
		if (!CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Failed to Setname on Swapchain\n");
		}
	}

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = dxgiFormat;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Texture2D.PlaneSlice = 0;

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_SwapchainRTVIndices[i] = m_pRTVDescriptorHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pRTVDescriptorHeap->GetCPUHeapAt(m_SwapchainRTVIndices[i]);
		
		m_pDevice5->CreateRenderTargetView(m_SwapchainResources[i], &rtvDesc, cdh);
	}

	// Create SRVs
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = dxgiFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_SwapchainSRVIndices[i] = m_pMainDescriptorHeap->GetNextDescriptorHeapIndex(1);
		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_pMainDescriptorHeap->GetCPUHeapAt(m_SwapchainSRVIndices[i]);
		m_pDevice5->CreateShaderResourceView(m_SwapchainResources[i], &srvDesc, cdh);
	}

#pragma endregion

	m_pGraphicsContext = new D3D12GraphicsContext(L"ManagerContext");

#pragma region CreateRootSignature

#pragma region SRVTABLE
	std::vector<D3D12_DESCRIPTOR_RANGE> dtRangesSRV;

	const unsigned int numSRVDescriptorRanges = 4;
	for (unsigned int i = 0; i < numSRVDescriptorRanges; i++)
	{
		D3D12_DESCRIPTOR_RANGE descriptorRange = {};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.NumDescriptors = -1;
		descriptorRange.BaseShaderRegister = 0;	// t0
		descriptorRange.RegisterSpace = i + 1;	// first range at space 1, space0 is for descriptors
		descriptorRange.OffsetInDescriptorsFromTableStart = 0;

		dtRangesSRV.push_back(descriptorRange);
	}

	D3D12_ROOT_DESCRIPTOR_TABLE dtSRV = {};
	dtSRV.NumDescriptorRanges = dtRangesSRV.size();
	dtSRV.pDescriptorRanges = dtRangesSRV.data();

#pragma endregion SRVTABLE

#pragma region CBVTABLE
	std::vector<D3D12_DESCRIPTOR_RANGE> dtRangesCBV;

	const unsigned int numCBVDescriptorRanges = 3;
	for (unsigned int i = 0; i < numCBVDescriptorRanges; i++)
	{
		D3D12_DESCRIPTOR_RANGE descriptorRange = {};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		descriptorRange.NumDescriptors = -1;
		descriptorRange.BaseShaderRegister = 0;	// b0
		descriptorRange.RegisterSpace = i + 1;	// first range at space 1, space0 is for descriptors
		descriptorRange.OffsetInDescriptorsFromTableStart = 0;

		dtRangesCBV.push_back(descriptorRange);
	}

	D3D12_ROOT_DESCRIPTOR_TABLE dtCBV = {};
	dtCBV.NumDescriptorRanges = dtRangesCBV.size();
	dtCBV.pDescriptorRanges = dtRangesCBV.data();

#pragma endregion CBVTABLE	

#pragma region UAVTABLE
	std::vector<D3D12_DESCRIPTOR_RANGE> dtRangesUAV;

	const unsigned int numUAVDescriptorRanges = 3;
	for (unsigned int i = 0; i < numUAVDescriptorRanges; i++)
	{
		D3D12_DESCRIPTOR_RANGE descriptorRange = {};
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		descriptorRange.NumDescriptors = -1;
		descriptorRange.BaseShaderRegister = 0;	// u0
		descriptorRange.RegisterSpace = i + 1;	// first range at space 1, space0 is for descriptors
		descriptorRange.OffsetInDescriptorsFromTableStart = 0;

		dtRangesUAV.push_back(descriptorRange);
	}

	D3D12_ROOT_DESCRIPTOR_TABLE dtUAV = {};
	dtUAV.NumDescriptorRanges = dtRangesUAV.size();
	dtUAV.pDescriptorRanges = dtRangesUAV.data();

#pragma endregion UAVTABLE

#pragma region SetTables
	D3D12_ROOT_PARAMETER rootParam[E_GLOBAL_ROOTSIGNATURE::NUM_PARAMS]{};

	rootParam[E_GLOBAL_ROOTSIGNATURE::dtCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtCBV].DescriptorTable = dtCBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::dtSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtSRV].DescriptorTable = dtSRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::dtUAV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtUAV].DescriptorTable = dtUAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::dtUAV].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#pragma endregion

#pragma region RootConstants
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_SlotInfo_B0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_SlotInfo_B0].Constants.ShaderRegister = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_SlotInfo_B0].Constants.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_SlotInfo_B0].Constants.Num32BitValues = sizeof(SlotInfo) / sizeof(UINT);
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_SlotInfo_B0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_DH_Indices_B1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_DH_Indices_B1].Constants.ShaderRegister = 1;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_DH_Indices_B1].Constants.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_DH_Indices_B1].Constants.Num32BitValues = sizeof(RootConstantUints) / sizeof(UINT);
	rootParam[E_GLOBAL_ROOTSIGNATURE::Constants_DH_Indices_B1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

#pragma region ROOTPARAMS_CBV
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B2].Descriptor.ShaderRegister = 2;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B2].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B3].Descriptor.ShaderRegister = 3;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B3].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B4].Descriptor.ShaderRegister = 4;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B4].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B5].Descriptor.ShaderRegister = 5;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B5].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// The following 2 shaderRegisters are free to use for custom constantBuffers
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B6].Descriptor.ShaderRegister = 6;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B6].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B7].Descriptor.ShaderRegister = 7;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B7].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_CBV_B7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#pragma endregion
#pragma endregion

#pragma region ROOTPARAMS_SRV
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T0].Descriptor.ShaderRegister = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T0].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T1].Descriptor.ShaderRegister = 1;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T1].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T2].Descriptor.ShaderRegister = 2;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T2].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T3].Descriptor.ShaderRegister = 3;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T3].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T4].Descriptor.ShaderRegister = 4;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T4].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T5].Descriptor.ShaderRegister = 5;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T5].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_SRV_T5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#pragma endregion

#pragma region ROOTPARAMS_UAV
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U0].Descriptor.ShaderRegister = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U0].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U1].Descriptor.ShaderRegister = 1;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U1].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U2].Descriptor.ShaderRegister = 2;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U2].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U3].Descriptor.ShaderRegister = 3;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U3].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U4].Descriptor.ShaderRegister = 4;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U4].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U5].Descriptor.ShaderRegister = 5;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U5].Descriptor.RegisterSpace = 0;
	rootParam[E_GLOBAL_ROOTSIGNATURE::RootParam_UAV_U5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
#pragma endregion

#pragma region StaticSamplers
	const unsigned int numStaticSamplers = 8;
	D3D12_STATIC_SAMPLER_DESC ssd[numStaticSamplers] = {};

	// Anisotropic Wrap
	for (unsigned int i = 0; i < 4; i++)
	{
		ssd[i].ShaderRegister = i;
		ssd[i].Filter = D3D12_FILTER_ANISOTROPIC;
		ssd[i].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		ssd[i].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		ssd[i].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		ssd[i].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		ssd[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		ssd[i].MinLOD = 0;
		ssd[i].MaxLOD = D3D12_FLOAT32_MAX;
		ssd[i].MaxAnisotropy = 2 * (i + 1);
	}

	ssd[4].ShaderRegister = 4;
	ssd[4].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	ssd[4].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	ssd[4].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	ssd[4].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	ssd[4].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	ssd[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	ssd[4].MinLOD = 0;
	ssd[4].MaxLOD = D3D12_FLOAT32_MAX;
	ssd[4].MipLODBias = 0.0f;
	ssd[4].MaxAnisotropy = 1;
	ssd[4].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;

	// BilinearWrap
	ssd[5].ShaderRegister = 5;
	ssd[5].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	ssd[5].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[5].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[5].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[5].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	ssd[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	ssd[5].MinLOD = 0;
	ssd[5].MaxLOD = D3D12_FLOAT32_MAX;
	ssd[5].MipLODBias = 0.0f;

	// BilinearClamp
	ssd[6].ShaderRegister = 6;
	ssd[6].Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	ssd[6].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	ssd[6].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	ssd[6].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	ssd[6].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	ssd[6].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	ssd[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	ssd[6].MinLOD = 0;
	ssd[6].MaxLOD = D3D12_FLOAT32_MAX;
	ssd[6].MipLODBias = 0.0f;

	// TrilinearWrap
	ssd[7].ShaderRegister = 7;
	ssd[7].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	ssd[7].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[7].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[7].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	ssd[7].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	ssd[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	ssd[7].MinLOD = 0;
	ssd[7].MaxLOD = D3D12_FLOAT32_MAX;
	ssd[7].MipLODBias = 0.0f;

	

#pragma endregion

#pragma region Create

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
	rsDesc.NumParameters = ARRAYSIZE(rootParam);
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = numStaticSamplers;
	rsDesc.pStaticSamplers = ssd;

#ifdef DEBUG
	unsigned int size = 0;
	for (const D3D12_ROOT_PARAMETER& param : rootParam)
	{
		if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			size += 1;
		}
		else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
		{
			size += param.Constants.Num32BitValues;
		}
		else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV ||
			param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_UAV ||
			param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV)
		{
			size += 2;
		}
	}

	if (size > 64)
	{
		BL_LOG_CRITICAL("RootSignature too big! Size: %d DWORDS\n", size);
	}
#endif

	ID3DBlob* errorMessages = nullptr;
	ID3DBlob* m_pBlob = nullptr;

	hr = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&m_pBlob,
		&errorMessages);

	if (FAILED(hr) && errorMessages)
	{
		BL_LOG_CRITICAL("Failed to Serialize RootSignature\n");

		const char* errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
		BL_LOG_CRITICAL("%s\n", errorMsg);
	}

	hr = m_pDevice5->CreateRootSignature(
		0,
		m_pBlob->GetBufferPointer(),
		m_pBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_pGlobalRootSig));

	if (FAILED(hr))
	{
		BL_ASSERT_MESSAGE(m_pGlobalRootSig != nullptr, "Failed to create RootSignature\n");
	}

	BL_SAFE_RELEASE(&m_pBlob);

	m_pGlobalRootSig->SetName(L"GlobalRootSig");
#pragma endregion

#pragma endregion

#pragma region CreateIntermediateUploadHeap
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.CreationNodeMask = 1; //used when multi-gpu
		heapProps.VisibleNodeMask = 1; //used when multi-gpu
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Width = m_IntermediateUploadHeapSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Create buffer
		hr = m_pDevice5->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,    // ClearValue, nullptr for buffers
			IID_PPV_ARGS(&m_pIntermediateUploadHeap[i])
		);
		if (!CHECK_HRESULT(hr))
		{
			BL_LOG_CRITICAL("Could not create D3D12IntermediateUploadHeap\n");
		}

		std::wstring resourceName = L"IntermediateUploadHeap" + std::to_wstring(i);
		m_pIntermediateUploadHeap[i]->SetName(resourceName.c_str());

		// Permanently mapped
		m_pIntermediateUploadHeap[i]->Map(0, nullptr, &m_pIntermediateUploadHeapBegin[i]);
	}
#pragma endregion
}

void D3D12GraphicsManager::Begin()
{

}

void D3D12GraphicsManager::End()
{
	// Delete D3D12 resources that are guaranteed not used anymore
	deleteDefferedDeletionObjects(false);

	mFrameIndex++;
	m_CommandInterfaceIndex = (m_CommandInterfaceIndex + 1) % NUM_SWAP_BUFFERS;

	// Reset the offset of the dynamic upload heap for next frame
	m_pIntermediateUploadHeapAtomicCurrentOffset = 0;
}

void D3D12GraphicsManager::ExecuteGraphicsContexts(const std::vector<IGraphicsContext*>& graphicsContexts, unsigned int numGraphicsContexts)
{
	std::vector<ID3D12CommandList*> cList;
	cList.reserve(numGraphicsContexts);
	
	for (IGraphicsContext* graphicsContext : graphicsContexts)
	{
		cList.push_back(static_cast<D3D12GraphicsContext*>(graphicsContext)->m_pCommandList);
	}

	m_pGraphicsCommandQueue->ExecuteCommandLists(numGraphicsContexts, cList.data());
}

void D3D12GraphicsManager::SyncAndPresent(IGraphicsTexture* finalColorTexture)
{
	auto TransferResourceState = [this](ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
	{
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Transition.Subresource = 0;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = stateBefore;
		barrier.Transition.StateAfter = stateAfter;

		ID3D12GraphicsCommandList5* tempCL = static_cast<D3D12GraphicsContext*>(m_pGraphicsContext)->m_pCommandList;

		tempCL->ResourceBarrier(1, &barrier);

	};
	// Copy to Swapchain
	m_pGraphicsContext->Begin();
	{
		ScopedPixEvent(CopyToSwapchain, m_pGraphicsContext);

		D3D12GraphicsTexture* d3d12FinalColorTexture = static_cast<D3D12GraphicsTexture*>(finalColorTexture);

		D3D12_TEXTURE_COPY_LOCATION copyDst = {};
		copyDst.pResource = m_SwapchainResources[m_CommandInterfaceIndex];
		copyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		copyDst.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION copySrc = {};
		copySrc.pResource = d3d12FinalColorTexture->m_pResource;
		copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		copySrc.SubresourceIndex = 0;

		TransferResourceState(d3d12FinalColorTexture->m_pResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		TransferResourceState(m_SwapchainResources[m_CommandInterfaceIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

		m_pGraphicsContext->m_pCommandList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);

		TransferResourceState(m_SwapchainResources[m_CommandInterfaceIndex], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		TransferResourceState(d3d12FinalColorTexture->m_pResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	}
	m_pGraphicsContext->End();

	// Execute the finalCList
	ID3D12CommandList* cList = m_pGraphicsContext->m_pCommandList;
	m_pGraphicsCommandQueue->ExecuteCommandLists(1, &cList);

	HRESULT hr = m_pSwapChain4->Present(0, 0);

	waitForFrame(NUM_SWAP_BUFFERS - 1);
	//waitForFrame(0);

	if(!CHECK_HRESULT(hr))
	{
		BL_LOG_CRITICAL("Swapchain Failed to present\n");
	}
}

bool D3D12GraphicsManager::CHECK_HRESULT(HRESULT hrParam)
{
	if (SUCCEEDED(hrParam))
		return true;

	std::string buffer;
	std::string message = std::system_category().message(hrParam);
	std::string deviceRemovedMessage;

	buffer = "\n\nSomething is horribly and badly designed! (HRESULT:" + std::to_string(hrParam) + "), " + message;

	if (hrParam == DXGI_ERROR_DEVICE_REMOVED)
	{
		ID3D12Device5* device5 = D3D12GraphicsManager::GetInstance()->GetDevice();
		if (device5 != nullptr)
		{
			HRESULT hrDevicedRemovedReason = device5->GetDeviceRemovedReason();
			deviceRemovedMessage = std::system_category().message(hrDevicedRemovedReason);
			std::string reason = "Reason: " + deviceRemovedMessage + "\n\n";
			buffer += reason;
		}
	}
	else if (hrParam == DXGI_ERROR_DEVICE_HUNG)
	{
		std::string reason = "Reason: DXGI_ERROR_DEVICE_HUNG\n\n";
		buffer += reason;
	}

	BL_LOG_CRITICAL(buffer.c_str());

	__debugbreak();

	return false;
}

void D3D12GraphicsManager::AddIUknownForDefferedDeletion(IUnknown* object)
{
	m_ObjectsToBeDeleted.push_back(std::make_pair(mFrameIndex, object));
}

DynamicDataParams D3D12GraphicsManager::SetDynamicData(unsigned int size, const void* data)
{
	// Makes this function threadsafe
	long offset = InterlockedAdd(&m_pIntermediateUploadHeapAtomicCurrentOffset, size);

	if (offset > m_IntermediateUploadHeapSize)
	{
		BL_LOG_CRITICAL("IntermediateUploadHeap to small");
		DynamicDataParams emptyParam = {};
		return emptyParam;
	}

	long currentOffset = offset - size;

	// Copy to CPU address
	void* cpuAddress = (char*)m_pIntermediateUploadHeapBegin[m_CommandInterfaceIndex] + currentOffset;
	memcpy(cpuAddress, data, size);

	DynamicDataParams returnVal = {};
	returnVal.offsetFromStart = currentOffset;
	returnVal.uploadResource = m_pIntermediateUploadHeap[m_CommandInterfaceIndex];
	returnVal.vAddr = m_pIntermediateUploadHeap[m_CommandInterfaceIndex]->GetGPUVirtualAddress() + currentOffset;
	return returnVal;
}

D3D12DescriptorHeap* D3D12GraphicsManager::GetMainDescriptorHeap() const
{
	return m_pMainDescriptorHeap;
}

D3D12DescriptorHeap* D3D12GraphicsManager::GetRTVDescriptorHeap() const
{
	return m_pRTVDescriptorHeap;
}

D3D12DescriptorHeap* D3D12GraphicsManager::GetDSVDescriptorHeap() const
{
	return m_pDSVDescriptorHeap;
}

void D3D12GraphicsManager::waitForFrame(unsigned int frameToWaitFor)
{
	static const unsigned int nrOfFenceChangesPerFrame = 1;
	
	unsigned int fenceValuesToBeAhead = frameToWaitFor * nrOfFenceChangesPerFrame;
	
	m_pGraphicsCommandQueue->Signal(m_pMainFence, m_MainFenceValue);
	//Wait if the CPU is "framesToBeAhead" number of frames ahead of the GPU
	if (m_pMainFence->GetCompletedValue() < m_MainFenceValue - fenceValuesToBeAhead)
	{
		m_pMainFence->SetEventOnCompletion(m_MainFenceValue - fenceValuesToBeAhead, m_MainFenceEventHandle);
		WaitForSingleObject(m_MainFenceEventHandle, INFINITE);
	}
	m_MainFenceValue++;
}

void D3D12GraphicsManager::waitForGPU(ID3D12CommandQueue* commandQueue)
{
	TODO("Make this function threadsafe");

	m_HardSynceFenceValue++;
	m_pHardSyncFence->SetEventOnCompletion(m_HardSynceFenceValue, m_HardSyncFenceEventHandle);

	commandQueue->Signal(m_pHardSyncFence, m_HardSynceFenceValue);

	WaitForSingleObject(m_HardSyncFenceEventHandle, INFINITE);	
}

void D3D12GraphicsManager::deleteDefferedDeletionObjects(bool forceDeleteAll)
{
	unsigned int framesToWait = forceDeleteAll ? 0 : NUM_SWAP_BUFFERS;
	{
		std::vector<std::pair<unsigned int, IUnknown*>> remainingIUnknowns(m_ObjectsToBeDeleted.size());

		unsigned int numObjectsToSave = 0;
		for (auto [frameDeleted, object] : m_ObjectsToBeDeleted)
		{
			if ((frameDeleted + framesToWait) == mFrameIndex)
			{
				BL_SAFE_RELEASE(&object);
			}
			else
			{
				remainingIUnknowns[numObjectsToSave] = std::pair(frameDeleted, object);
				numObjectsToSave++;
			}
		}

		m_ObjectsToBeDeleted = remainingIUnknowns;
	}
}
