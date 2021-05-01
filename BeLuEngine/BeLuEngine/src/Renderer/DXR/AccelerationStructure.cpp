#include "stdafx.h"
#include "AccelerationStructure.h"

#include "../GPUMemory/Resource.h"

AccelerationStructure::AccelerationStructure()
{
}

AccelerationStructure::~AccelerationStructure()
{
	SAFE_DELETE(m_pScratch);
	SAFE_DELETE(m_pResult);
}

const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& AccelerationStructure::GetBuildDesc() const
{
	return m_BuildDesc;
}

void AccelerationStructure::BuildAccelerationStructure(ID3D12GraphicsCommandList4* commandList) const
{
	commandList->BuildRaytracingAccelerationStructure(&m_BuildDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = m_pResult->GetID3D12Resource1();
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	commandList->ResourceBarrier(1, &uavBarrier);
}
