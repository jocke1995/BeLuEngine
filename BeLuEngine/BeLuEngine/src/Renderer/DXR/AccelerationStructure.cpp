#include "stdafx.h"
#include "AccelerationStructure.h"

#include "../Misc/Log.h"

#include "../API/D3D12/D3D12GraphicsBuffer.h"
AccelerationStructure::AccelerationStructure()
{
}

AccelerationStructure::~AccelerationStructure()
{
	BL_SAFE_DELETE(m_pScratchBuffer);
	BL_SAFE_DELETE(m_pResultBuffer);
}

void AccelerationStructure::BuildAccelerationStructure(ID3D12GraphicsCommandList4* commandList) const
{
	commandList->BuildRaytracingAccelerationStructure(&m_BuildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->GetTempResource();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &uavBarrier);
}

IGraphicsBuffer* AccelerationStructure::GetRayTracingResultBuffer() const
{
	BL_ASSERT(m_pResultBuffer);
	return m_pResultBuffer;
}
