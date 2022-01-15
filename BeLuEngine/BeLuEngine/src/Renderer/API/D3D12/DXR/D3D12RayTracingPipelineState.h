#ifndef D3D12RAYTRACINGPIPELINESTATE_H
#define D3D12RAYTRACINGPIPELINESTATE_H

// Took inspiration from 
// https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1

#include "../../Interface/RayTracing/IRayTracingPipelineState.h"

struct ID3D12RootSignature;

class D3D12RayTracingPipelineState : public IRayTracingPipelineState
{
public:
    D3D12RayTracingPipelineState(const RayTracingPipelineStateDesc& desc, const std::wstring& name);
    virtual ~D3D12RayTracingPipelineState();

    ID3D12StateObjectProperties* GetTempStatePropsObject() { return m_pRTStateObjectProps; }
private:
    friend class D3D12GraphicsContext;
    friend class D3D12ShaderBindingTable;

    ID3D12StateObject* m_pRayTracingStateObject = nullptr;
    ID3D12StateObjectProperties* m_pRTStateObjectProps = nullptr;

    ID3D12RootSignature* createLocalRootSignature(const RayTracingRootSignatureAssociation& rootSigAssociation);
    std::vector<ID3D12RootSignature*> m_RootSigs;
};

#endif
