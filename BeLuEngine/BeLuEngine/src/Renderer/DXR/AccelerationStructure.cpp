#include "stdafx.h"
#include "AccelerationStructure.h"

#include "../API/IGraphicsBuffer.h"

AccelerationStructure::AccelerationStructure()
{
}

AccelerationStructure::~AccelerationStructure()
{
	BL_SAFE_DELETE(m_pScratchBuffer);
	BL_SAFE_DELETE(m_pResultBuffer);
}

IGraphicsBuffer* AccelerationStructure::GetRayTracingResultBuffer() const
{
	BL_ASSERT(m_pResultBuffer);
	return m_pResultBuffer;
}

const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& AccelerationStructure::GetBuildDesc() const
{
	return m_BuildDesc;
}
