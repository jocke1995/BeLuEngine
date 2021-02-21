#include "stdafx.h"
#include "RenderData.h"

EngineStatistics::EngineStatistics()
{
}

EngineStatistics::~EngineStatistics()
{
}

IM_CommonStats& EngineStatistics::GetIM_CommonStats()
{
	return m_CommonInfo;
}

IM_MemoryStats& EngineStatistics::GetIM_MemoryStats()
{
	return m_MemoryInfo;
}

std::vector<IM_ThreadStats*>& EngineStatistics::GetIM_ThreadStats()
{
	return m_ThreadInfo;
}