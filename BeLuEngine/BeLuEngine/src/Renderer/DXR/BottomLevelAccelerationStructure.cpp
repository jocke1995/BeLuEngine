#include "stdafx.h"
#include "BottomLevelAccelerationStructure.h"

#include "../GPUMemory/Resource.h"

// For the sizeof(Vertex)
#include "../Geometry/Mesh.h"

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure()
{
}

BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
{
}

void BottomLevelAccelerationStructure::AddVertexBuffer(
	ID3D12Resource* vertexBuffer, uint32_t vertexCount,
	ID3D12Resource* indexBuffer, uint32_t indexCount,
	bool isOpaque)
{
	D3D12_RAYTRACING_GEOMETRY_DESC rtGeometryDesc = {};

	rtGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	rtGeometryDesc.Flags = isOpaque ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

	// Vertex Buffer
	rtGeometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
	rtGeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	rtGeometryDesc.Triangles.VertexCount = vertexCount;

	// Index Buffer
	rtGeometryDesc.Triangles.IndexBuffer = vertexBuffer->GetGPUVirtualAddress();
	rtGeometryDesc.Triangles.IndexCount = indexCount;

	m_vertexBuffers.push_back(rtGeometryDesc);
}

void BottomLevelAccelerationStructure::FinalizeAccelerationStructure(ID3D12Device5* pDevice)
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc;
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = static_cast<UINT>(m_vertexBuffers.size());
	prebuildDesc.pGeometryDescs = m_vertexBuffers.data();
	prebuildDesc.Flags = flags;

	// This structure is used to hold the sizes of the required scratch memory and
	// resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

	// Buffer sizes need to be 256-byte-aligned
	unsigned int scratchSizeInBytes = (info.ScratchDataSizeInBytes + 255) & ~255;
	unsigned int resultSizeInBytes  = (info.ResultDataMaxSizeInBytes + 255) & ~255;

	m_BuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	m_BuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	m_BuildDesc.Inputs.NumDescs = static_cast<UINT>(m_vertexBuffers.size());
	m_BuildDesc.Inputs.pGeometryDescs = m_vertexBuffers.data();
	m_BuildDesc.DestAccelerationStructureData = { m_pResult->GetID3D12Resource1()->GetGPUVirtualAddress() };
	m_BuildDesc.ScratchAccelerationStructureData = { m_pScratch->GetID3D12Resource1()->GetGPUVirtualAddress() };
	m_BuildDesc.SourceAccelerationStructureData =  0;

	m_BuildDesc.Inputs.Flags = flags;
}
