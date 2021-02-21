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

// Singleton to hold all debug info
class EngineStatistics
{
public:
	virtual ~EngineStatistics();

	static IM_CommonStats& GetIM_RenderStats();
private:
	EngineStatistics();

	// Structs with statistics to draw
	static inline IM_CommonStats m_DebugInfo = {};
};

#endif
