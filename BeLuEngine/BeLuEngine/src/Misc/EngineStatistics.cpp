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

D3D12ContextStats D3D12ContextStats::operator+(const D3D12ContextStats& other) const
{
	D3D12ContextStats result = {};

	// Set DescriptorHeap
	result.m_NumSetDescriptorHeaps				= this->m_NumSetDescriptorHeaps				+ other.m_NumSetDescriptorHeaps;
	result.m_NumSetComputeRootSignature			= this->m_NumSetComputeRootSignature		+ other.m_NumSetComputeRootSignature;
	result.m_NumSetComputeRootDescriptorTable	= this->m_NumSetComputeRootDescriptorTable	+ other.m_NumSetComputeRootDescriptorTable;
	result.m_NumSetGraphicsRootSignature		= this->m_NumSetGraphicsRootSignature		+ other.m_NumSetGraphicsRootSignature;
	result.m_NumSetGraphicsRootDescriptorTable	= this->m_NumSetGraphicsRootDescriptorTable + other.m_NumSetGraphicsRootDescriptorTable;

	// Copy
	result.m_NumCopyBufferRegion	= this->m_NumCopyBufferRegion	+ other.m_NumCopyBufferRegion;
	result.m_NumCopyTextureRegion	= this->m_NumCopyTextureRegion	+ other.m_NumCopyTextureRegion;

	// Misc
	result.m_NumSetPipelineState		= this->m_NumSetPipelineState		+ other.m_NumSetPipelineState;
	result.m_NumIASetPrimitiveTopology	= this->m_NumIASetPrimitiveTopology + other.m_NumIASetPrimitiveTopology;
	result.m_NumResourceBarriers		= this->m_NumResourceBarriers		+ other.m_NumResourceBarriers;
	result.m_NumRSSetViewports			= this->m_NumRSSetViewports			+ other.m_NumRSSetViewports;
	result.m_NumRSSetScissorRects		= this->m_NumRSSetScissorRects		+ other.m_NumRSSetScissorRects;
	result.m_NumOMSetStencilRef			= this->m_NumOMSetStencilRef		+ other.m_NumOMSetStencilRef;

	// RenderTarget and Clears
	result.m_NumOMSetRenderTargets		= this->m_NumOMSetRenderTargets		+ other.m_NumOMSetRenderTargets;
	result.m_NumClearDepthStencilView	= this->m_NumClearDepthStencilView	+ other.m_NumClearDepthStencilView;
	result.m_NumClearRenderTargetView	= this->m_NumClearRenderTargetView	+ other.m_NumClearRenderTargetView;
	result.m_NumClearUnorderedAccessViewFloat	= this->m_NumClearUnorderedAccessViewFloat	+ other.m_NumClearUnorderedAccessViewFloat;
	result.m_NumClearUnorderedAccessViewUint	= this->m_NumClearUnorderedAccessViewUint	+ other.m_NumClearUnorderedAccessViewUint;

	// Views
	result.m_NumSetComputeRootShaderResourceView	= this->m_NumSetComputeRootShaderResourceView	+ other.m_NumSetComputeRootShaderResourceView;
	result.m_NumSetGraphicsRootShaderResourceView	= this->m_NumSetGraphicsRootShaderResourceView	+ other.m_NumSetGraphicsRootShaderResourceView;
	result.m_NumSetComputeRootConstantBufferView	= this->m_NumSetComputeRootConstantBufferView	+ other.m_NumSetComputeRootConstantBufferView;
	result.m_NumSetGraphicsRootConstantBufferView	= this->m_NumSetGraphicsRootConstantBufferView	+ other.m_NumSetGraphicsRootConstantBufferView;
	result.m_NumSetComputeRoot32BitConstants		= this->m_NumSetComputeRoot32BitConstants		+ other.m_NumSetComputeRoot32BitConstants;
	result.m_NumSetGraphicsRoot32BitConstants		= this->m_NumSetGraphicsRoot32BitConstants		+ other.m_NumSetGraphicsRoot32BitConstants;

	// Draw and Dispatch
	result.m_NumDrawIndexedInstanced	= this->m_NumDrawIndexedInstanced	+ other.m_NumDrawIndexedInstanced;
	result.m_NumDispatch				= this->m_NumDispatch				+ other.m_NumDispatch;

	// Raytracing
	result.m_NumBuildTLAS		= this->m_NumBuildTLAS		+ other.m_NumBuildTLAS;
	result.m_NumBuildBLAS		= this->m_NumBuildBLAS		+ other.m_NumBuildBLAS;
	result.m_NumDispatchRays	= this->m_NumDispatchRays	+ other.m_NumDispatchRays;
	result.m_NumSetRayTracingPipelineState = this->m_NumSetRayTracingPipelineState + other.m_NumSetRayTracingPipelineState;

	return result;
}
