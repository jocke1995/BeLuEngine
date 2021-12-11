#include "stdafx.h"
#include "BottomLevelAccelerationStructure.h"

#include "../Misc/Log.h"

// For the sizeof(Vertex)
#include "../Geometry/Mesh.h"

// TODO ABSTRACTION
#include "../API/D3D12/D3D12GraphicsManager.h"
#include "../API/D3D12/D3D12GraphicsBuffer.h"
#include "../API/D3D12/D3D12GraphicsTexture.h"

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure()
{
}

BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
{
}

void BottomLevelAccelerationStructure::AddVertexBuffer(
	IGraphicsBuffer* vertexBuffer, uint32_t vertexCount,
	IGraphicsBuffer* indexBuffer, uint32_t indexCount)
{
	D3D12_RAYTRACING_GEOMETRY_DESC rtGeometryDesc = {};

	// TODO: Add flexibility with rayGen and transparent objects
	rtGeometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	rtGeometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Vertex Buffer
	rtGeometryDesc.Triangles.VertexBuffer.StartAddress = static_cast<D3D12GraphicsBuffer*>(vertexBuffer)->GetTempResource()->GetGPUVirtualAddress();
	rtGeometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
	rtGeometryDesc.Triangles.VertexCount = vertexCount;
	rtGeometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;

	// Index Buffer
	rtGeometryDesc.Triangles.IndexBuffer = static_cast<D3D12GraphicsBuffer*>(indexBuffer)->GetTempResource()->GetGPUVirtualAddress();
	rtGeometryDesc.Triangles.IndexCount = indexCount;
	rtGeometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;

	m_vertexBuffers.push_back(rtGeometryDesc);
}

void BottomLevelAccelerationStructure::Reset()
{
}

void BottomLevelAccelerationStructure::GenerateBuffers()
{
	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Cause of crash? How to update BottomLevel efficiently?
	//flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;


	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc;
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = m_vertexBuffers.size();
	prebuildDesc.pGeometryDescs = m_vertexBuffers.data();
	prebuildDesc.Flags = flags;

	// This structure is used to hold the sizes of the required scratch memory and
	// resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

	// Buffer sizes need to be 256-byte-aligned
	unsigned int scratchSizeInBytes = (info.ScratchDataSizeInBytes   + 255) & ~255;
	unsigned int resultSizeInBytes  = (info.ResultDataMaxSizeInBytes + 255) & ~255;

	static unsigned int idCounter = 0;
	m_pScratchBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::UnorderedAccessBuffer, scratchSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"SCRATCHBUFFER_TLAS");
	m_pResultBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RayTracingBuffer, resultSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"RESULTBUFFER_TLAS");

	idCounter++;
}

void BottomLevelAccelerationStructure::SetupAccelerationStructureForBuilding(bool update)
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	flags |= update ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	m_BuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	m_BuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	m_BuildDesc.Inputs.NumDescs = static_cast<UINT>(m_vertexBuffers.size());
	m_BuildDesc.Inputs.pGeometryDescs = m_vertexBuffers.data();
	m_BuildDesc.DestAccelerationStructureData = { static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->GetTempResource()->GetGPUVirtualAddress() };
	m_BuildDesc.ScratchAccelerationStructureData = { static_cast<D3D12GraphicsBuffer*>(m_pScratchBuffer)->GetTempResource()->GetGPUVirtualAddress() };
	m_BuildDesc.SourceAccelerationStructureData = 0;

	m_BuildDesc.Inputs.Flags = flags;
}
