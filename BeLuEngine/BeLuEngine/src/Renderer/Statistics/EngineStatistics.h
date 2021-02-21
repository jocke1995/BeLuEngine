#ifndef ENGINESTATISTICS_H
#define ENGINESTATISTICS_H
#include "Core.h"

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
//
//struct IM_DirectX12Stats
//{
//	unsigned int m_NumUniqueCommandLists = 0;
//	unsigned int m_NumUniqueCommandAllocators = 0;
//
//	unsigned int m_NumExecuteCommandListsDirect = 0;
//};

// Singleton to hold all debug info
class EngineStatistics
{
public:
	virtual ~EngineStatistics();
	static EngineStatistics& GetInstance();

	static IM_CommonStats& GetIM_CommonStats();
	static IM_MemoryStats& GetIM_MemoryStats();
	static std::vector<IM_ThreadStats*>& GetIM_ThreadStats();
private:
	EngineStatistics();

	// Structs with statistics to draw
	static inline IM_CommonStats m_CommonInfo = {};
	static inline IM_MemoryStats m_MemoryInfo = {};
	static inline std::vector<IM_ThreadStats*> m_ThreadInfo = {};
};

#endif
