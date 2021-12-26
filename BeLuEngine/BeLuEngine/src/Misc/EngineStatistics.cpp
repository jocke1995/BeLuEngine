#include "stdafx.h"
#include "EngineStatistics.h"

#include "../Misc/MicrosoftCPU.h"

EngineStatistics::EngineStatistics()
{
	m_CommonInfo.m_CPU = InstructionSet::GetInstance().GetCPU();
}

EngineStatistics::~EngineStatistics()
{
}

EngineStatistics& EngineStatistics::GetInstance()
{
	static EngineStatistics instance;

	return instance;
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
