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

void EngineStatistics::BeginFrame()
{
	// Clear the per-frame-statistics for this frame
	m_CommonInfo = {};
	m_MemoryInfo = {};

	for (unsigned int i = 0; i < m_ThreadInfo.size(); i++)
	{
		m_ThreadInfo[i]->m_TasksCompleted = 0;
	}
	m_D3D12ContextStats = {};
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

D3D12Stats& EngineStatistics::GetD3D12ContextStats()
{
	return m_D3D12ContextStats;
}

void D3D12Stats::operator+=(const D3D12Stats& other)
{
	// Set DescriptorHeap
	this->m_NumSetDescriptorHeaps				+= other.m_NumSetDescriptorHeaps;
	this->m_NumSetComputeRootSignature			+= other.m_NumSetComputeRootSignature;
	this->m_NumSetComputeRootDescriptorTable	+= other.m_NumSetComputeRootDescriptorTable;
	this->m_NumSetGraphicsRootSignature			+= other.m_NumSetGraphicsRootSignature;
	this->m_NumSetGraphicsRootDescriptorTable	+= other.m_NumSetGraphicsRootDescriptorTable;

	// Copy
	this->m_NumCopyBufferRegion		+= other.m_NumCopyBufferRegion;
	this->m_NumCopyTextureRegion	+= other.m_NumCopyTextureRegion;

	// Misc
	this->m_NumSetPipelineState			+= other.m_NumSetPipelineState;
	this->m_NumIASetPrimitiveTopology	+= other.m_NumIASetPrimitiveTopology;
	this->m_NumLocalTransitionBarriers	+= other.m_NumLocalTransitionBarriers;
	this->m_NumGlobalTransitionBarriers	+= other.m_NumGlobalTransitionBarriers;
	this->m_NumUAVBarriers				+= other.m_NumUAVBarriers;
	this->m_NumRSSetViewports			+= other.m_NumRSSetViewports;
	this->m_NumRSSetScissorRects		+= other.m_NumRSSetScissorRects;
	this->m_NumOMSetStencilRef			+= other.m_NumOMSetStencilRef;

	// RenderTarget and Clears
	this->m_NumOMSetRenderTargets		+= other.m_NumOMSetRenderTargets;
	this->m_NumClearDepthStencilView	+= other.m_NumClearDepthStencilView;
	this->m_NumClearRenderTargetView	+= other.m_NumClearRenderTargetView;
	this->m_NumClearUnorderedAccessViewFloat	+= other.m_NumClearUnorderedAccessViewFloat;
	this->m_NumClearUnorderedAccessViewUint		+= other.m_NumClearUnorderedAccessViewUint;

	// Views
	this->m_NumSetComputeRootShaderResourceView		+= other.m_NumSetComputeRootShaderResourceView;
	this->m_NumSetGraphicsRootShaderResourceView	+= other.m_NumSetGraphicsRootShaderResourceView;
	this->m_NumSetComputeRootConstantBufferView		+= other.m_NumSetComputeRootConstantBufferView;
	this->m_NumSetGraphicsRootConstantBufferView	+= other.m_NumSetGraphicsRootConstantBufferView;
	this->m_NumSetComputeRoot32BitConstants			+= other.m_NumSetComputeRoot32BitConstants;
	this->m_NumSetGraphicsRoot32BitConstants		+= other.m_NumSetGraphicsRoot32BitConstants;

	// Draw and Dispatch
	this->m_NumIASetIndexBuffer		+= other.m_NumIASetIndexBuffer;
	this->m_NumDrawIndexedInstanced	+= other.m_NumDrawIndexedInstanced;
	this->m_NumDispatch				+= other.m_NumDispatch;

	// Raytracing
	this->m_NumBuildTLAS		+= other.m_NumBuildTLAS;
	this->m_NumBuildBLAS		+= other.m_NumBuildBLAS;
	this->m_NumDispatchRays		+= other.m_NumDispatchRays;
	this->m_NumSetRayTracingPipelineState += other.m_NumSetRayTracingPipelineState;
}
