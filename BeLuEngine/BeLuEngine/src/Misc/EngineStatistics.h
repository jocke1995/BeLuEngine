#ifndef ENGINESTATISTICS_H
#define ENGINESTATISTICS_H
#include "Core.h"

TODO("Wrap all of this in editorMode?");
struct IM_CommonStats
{
	std::string m_Build = "";
	bool m_DebugLayerActive = false;
	std::string m_API = "";
	bool m_STRenderer = SINGLE_THREADED_RENDERER;
	std::string m_Adapter = "";
	std::string m_CPU = "";
	unsigned int m_NumCpuCores = 0;
	unsigned int m_ResX, m_ResY = 0;
	unsigned int m_TotalFPS = 0;
	double m_TotalMS = 0.0f;
};

struct IM_MemoryStats
{
	// RAM
	unsigned int m_ProcessRamUsage = 0;
	unsigned int m_TotalRamUsage = 0;
	unsigned int m_TotalRam = 0;

	// VRAM
	unsigned int m_ProcessVramUsage = 0;
	unsigned int m_TotalVram = 0;
};

struct IM_ThreadStats
{
	unsigned int m_Id = 0;
	unsigned int m_TasksCompleted = 0;
};

//struct IM_RenderStats
//{
//	// Lights
//	unsigned int m_NumPointLights = 0;
//	unsigned int m_NumSpotLights = 0;
//	unsigned int m_NumDirectionalLights = 0;
//
//	unsigned int m_NumShadowCastingLights = 0;
//
//	// Draws
//	unsigned int m_NumDrawCalls = 0;
//	unsigned int m_NumVertices = 0;
//};

struct D3D12Stats
{
	friend class D3D12GraphicsContext;

	void operator+=(const D3D12Stats& other);

	// Updated by mainThread when resolving
	// Not touched by the overloaded operator+
	unsigned int m_NumUniqueCommandLists = 0;
	unsigned int m_NumExecuteCommandLists = 0;

	// Set DescriptorHeap
	unsigned int m_NumSetDescriptorHeaps = 0;
	unsigned int m_NumSetComputeRootSignature = 0;
	unsigned int m_NumSetComputeRootDescriptorTable = 0;
	unsigned int m_NumSetGraphicsRootSignature = 0;
	unsigned int m_NumSetGraphicsRootDescriptorTable = 0;

	// Copy
	unsigned int m_NumCopyBufferRegion = 0;
	unsigned int m_NumCopyTextureRegion = 0;

	// Misc
	unsigned int m_NumSetPipelineState = 0;
	unsigned int m_NumIASetPrimitiveTopology = 0;
	unsigned int m_NumLocalTransitionBarriers = 0;
	unsigned int m_NumGlobalTransitionBarriers = 0;
	unsigned int m_NumUAVBarriers = 0;
	unsigned int m_NumRSSetViewports = 0;
	unsigned int m_NumRSSetScissorRects = 0;
	unsigned int m_NumOMSetStencilRef = 0;

	// RenderTarget and Clears
	unsigned int m_NumOMSetRenderTargets = 0;
	unsigned int m_NumClearDepthStencilView = 0;
	unsigned int m_NumClearRenderTargetView = 0;
	unsigned int m_NumClearUnorderedAccessViewFloat = 0;
	unsigned int m_NumClearUnorderedAccessViewUint = 0;

	// Views
	unsigned int m_NumSetComputeRootShaderResourceView = 0;
	unsigned int m_NumSetGraphicsRootShaderResourceView = 0;
	unsigned int m_NumSetComputeRootConstantBufferView = 0;
	unsigned int m_NumSetGraphicsRootConstantBufferView = 0;
	unsigned int m_NumSetComputeRoot32BitConstants = 0;
	unsigned int m_NumSetGraphicsRoot32BitConstants = 0;

	// Draw and Dispatch
	unsigned int m_NumIASetIndexBuffer = 0;
	unsigned int m_NumDrawIndexedInstanced = 0;
	unsigned int m_NumDispatch = 0;

	// Raytracing
	unsigned int m_NumBuildTLAS = 0;
	unsigned int m_NumBuildBLAS = 0;
	unsigned int m_NumDispatchRays = 0;
	unsigned int m_NumSetRayTracingPipelineState = 0;
};

TODO("Wrap in a EditorMode define");
#define BL_EDITOR_MODE_APPEND(container, value) container += value;

// Singleton to hold all debug info
class EngineStatistics
{
public:
	virtual ~EngineStatistics();
	static EngineStatistics& GetInstance();

	static void BeginFrame();

	static IM_CommonStats& GetIM_CommonStats();
	static IM_MemoryStats& GetIM_MemoryStats();
	static std::vector<IM_ThreadStats*>& GetIM_ThreadStats();
	static D3D12Stats& GetD3D12ContextStats();
private:
	EngineStatistics();

	// Structs with statistics to draw
	static inline IM_CommonStats m_CommonInfo = {};
	static inline IM_MemoryStats m_MemoryInfo = {};
	static inline std::vector<IM_ThreadStats*> m_ThreadInfo = {};
	static inline D3D12Stats m_D3D12ContextStats = {};
};

#endif
