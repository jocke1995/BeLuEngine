#ifndef RENDERDATA_H
#define RENDERDATA_H

struct IM_RenderStats
{
	std::string m_API = "";
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

	static IM_RenderStats& GetIM_RenderStats();
private:
	EngineStatistics();

	// Structs with statistics to draw
	static inline IM_RenderStats m_DebugInfo = {};
};

#endif
