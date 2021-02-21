#include "stdafx.h"
#include "RenderData.h"

EngineStatistics::EngineStatistics()
{
}

EngineStatistics::~EngineStatistics()
{
}

IM_RenderStats& EngineStatistics::GetIM_RenderStats()
{
	return m_DebugInfo;
}
