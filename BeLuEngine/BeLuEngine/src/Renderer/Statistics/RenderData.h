#ifndef RENDERDATA_H
#define RENDERDATA_H
#include "Core.h"

struct IM_CommonStats
{
	std::string m_Build = "";
	bool m_DebugLayerActive = false;
	std::string m_API = "";
	bool m_STRenderer = SINGLE_THREADED_RENDERER;
	std::string m_Adapter = "";
	unsigned int m_ResX, m_ResY = 0;
	unsigned int m_TotalFPS = 0;
	double m_TotalMS;
};

struct IM_MemoryStats
{
	// RAM
	unsigned int m_ProcessRamUsage = 0;
	unsigned int m_TotalRamUsage = 0;
	unsigned int m_TotalRam = 0;

	// VRAM
	unsigned int m_ProcessVramUsage = 0;
	unsigned int m_CurrVramUsage = 0;
	unsigned int m_TotalVram = 0;
};

// Singleton to hold all debug info
class EngineStatistics
{
public:
	virtual ~EngineStatistics();

	static IM_CommonStats& GetIM_CommonStats();
	static IM_MemoryStats& GetIM_MemoryStats();
private:
	EngineStatistics();

	// Structs with statistics to draw
	static inline IM_CommonStats m_CommonInfo = {};
	static inline IM_MemoryStats m_MemoryInfo = {};
};

#endif
