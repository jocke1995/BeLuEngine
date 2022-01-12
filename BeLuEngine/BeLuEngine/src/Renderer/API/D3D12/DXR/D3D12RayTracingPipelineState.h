#ifndef D3D12RAYTRACINGPIPELINESTATE_H
#define D3D12RAYTRACINGPIPELINESTATE_H

#include "../../Interface/RayTracing/IRayTracingPipelineState.h"

struct ID3D12RootSignature;

class D3D12RayTracingPipelineState : public IRayTracingPipelineState
{
public:
    D3D12RayTracingPipelineState(const RayTracingPSDesc& desc, const std::wstring& name);
    virtual ~D3D12RayTracingPipelineState();


private:
    friend class D3D12Context;
    ID3D12StateObject* m_pRayTracingState = nullptr;

    ID3D12RootSignature* createLocalRootSignature(const std::vector<IRayTracingRootSignatureParams>& rootSigParams);
};

#endif
