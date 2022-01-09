#ifndef D3D12RAYTRACINGPIPELINESTATE_H
#define D3D12RAYTRACINGPIPELINESTATE_H

#include "../../Interface/RayTracing/IRayTracingPipelineState.h"

class D3D12RayTracingPipelineState : public IRayTracingPipelineState
{
public:
    D3D12RayTracingPipelineState(const RayTracingPSDesc& desc, const std::wstring& name);
    virtual ~D3D12RayTracingPipelineState();


private:
   
};

#endif
