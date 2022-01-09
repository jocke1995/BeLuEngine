#ifndef IRAYTRACINGPIPELINESTATE_H
#define IRAYTRACINGPIPELINESTATE_H

#include "RenderCore.h"

class RayTracingPSDesc
{
public:
	RayTracingPSDesc();
	virtual ~RayTracingPSDesc();

private:
    friend class D3D12RayTracingPipelineState;

    std::vector<std::wstring> m_Shaders;
};

class IRayTracingPipelineState
{
public:
	virtual ~IRayTracingPipelineState();

	static IRayTracingPipelineState* Create(const RayTracingPSDesc& desc, const std::wstring& name);

protected:

#ifdef DEBUG
	std::wstring m_DebugName = L"";
#endif
};

#endif