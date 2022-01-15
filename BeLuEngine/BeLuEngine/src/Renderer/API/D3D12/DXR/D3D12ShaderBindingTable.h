#ifndef D3D12SHADERBINDINGTABLE_H
#define D3D12SHADERBINDINGTABLE_H

// Took inspiration from 
// https://developer.nvidia.com/rtx/raytracing/dxr/dx12-raytracing-tutorial-part-1

#include "../../Interface/RayTracing/IShaderBindingTable.h"

class IGraphicsBuffer;
class IRayTracingPipelineState;
class D3D12RayTracingPipelineState;

class D3D12ShaderBindingTable : public IShaderBindingTable
{
public:
    D3D12ShaderBindingTable(const std::wstring& name);
    virtual ~D3D12ShaderBindingTable();

    virtual void GenerateShaderBindingTable(IRayTracingPipelineState* rtPipelineState) override final;

private:
    friend class D3D12GraphicsContext;

    IGraphicsBuffer* m_pCPUShaderTableBuffer = nullptr;

    unsigned int calculateShaderBindingTableSize();

    unsigned int copyShaderRecords(
        D3D12RayTracingPipelineState* d3d12RayTracingPipelineState,
        unsigned char* pMappedData,
        const std::vector<ShaderRecord>& shaderRecords,
        unsigned int maxShaderRecordSize);
};

#endif
