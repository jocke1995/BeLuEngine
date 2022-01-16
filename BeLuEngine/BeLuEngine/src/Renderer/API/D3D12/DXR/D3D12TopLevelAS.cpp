#include "stdafx.h"
#include "D3D12TopLevelAS.h"

// For the InstanceMask flagstruct
#include "GPU_Structs.h"

#include "D3D12BottomLevelAS.h"

// D3D12
#include "../D3D12GraphicsManager.h"
#include "../D3D12GraphicsBuffer.h"
#include "../D3D12GraphicsTexture.h"

D3D12TopLevelAS::D3D12TopLevelAS()
{
	
}

D3D12TopLevelAS::~D3D12TopLevelAS()
{
	BL_SAFE_DELETE(m_pScratchBuffer);
	BL_SAFE_DELETE(m_pResultBuffer);
}

bool D3D12TopLevelAS::CreateResultBuffer()
{
	// Only generate new buffers if we added new objects and the current one is to small
	// This buffer is only increasing in size, never shrinking.
	if (m_ResultBufferMaxNumberOfInstances < m_Instances.size())
	{
		D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
		ID3D12Device5* device5 = manager->GetDevice();

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		m_ResultBufferMaxNumberOfInstances = m_Instances.size();

		// Describe the work being requested, in this case the construction of a
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
		prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		prebuildDesc.NumDescs = m_ResultBufferMaxNumberOfInstances;
		prebuildDesc.Flags = flags;

		// This structure is used to hold the sizes of the required scratch memory and resulting AS
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		device5->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

		// Buffer sizes need to be 256-byte-aligned
		unsigned int scratchSizeInBytes = (info.ScratchDataSizeInBytes + 255) & ~255;
		unsigned int resultSizeInBytes = (info.ResultDataMaxSizeInBytes + 255) & ~255;

		BL_SAFE_DELETE(m_pScratchBuffer);
		BL_SAFE_DELETE(m_pResultBuffer);

		// Create new buffers for scratch and result
		m_pScratchBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::UnorderedAccessBuffer, scratchSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"SCRATCHBUFFER_TLAS");
		m_pResultBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RayTracingBuffer, resultSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"RESULTBUFFER_TLAS");

		return true;
	}

	return false;
}

IGraphicsBuffer* D3D12TopLevelAS::GetRayTracingResultBuffer() const
{
	BL_ASSERT(m_pResultBuffer);
	return m_pResultBuffer;
}

void D3D12TopLevelAS::MapResultBuffer()
{
	BL_ASSERT(m_pResultBuffer && m_pScratchBuffer);

	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();

	unsigned int numInstances = m_Instances.size();

	TODO("Don't do this on the heap every frame.. Create a stack allocator!");
	unsigned int sizeInBytes = numInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = new D3D12_RAYTRACING_INSTANCE_DESC[numInstances];

	// Create the description for each instance
	for (unsigned int i = 0; i < numInstances; i++)
	{
		instanceDescs[i].InstanceID = m_Instances[i].m_ID;
		instanceDescs[i].InstanceContributionToHitGroupIndex = m_Instances[i].m_ID;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		DirectX::XMMATRIX m = XMMatrixTranspose(m_Instances[i].m_Transform);
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = static_cast<D3D12GraphicsBuffer*>(m_Instances[i].m_pBLAS)->m_pResource->GetGPUVirtualAddress();

		// Objects who doesn't give shadows doesn't get this maskFlag added to them.
		if (m_Instances[i].m_GiveShadows)
		{
			instanceDescs[i].InstanceMask = INSTANCE_MASK_SCENE_MINUS_NOSHADOWOBJECTS;
		}

		instanceDescs[i].InstanceMask |= INSTANCE_MASK_ENTIRE_SCENE;
	}

	DynamicDataParams dynamicDataParams = manager->SetDynamicData(sizeInBytes, instanceDescs);
	delete[] instanceDescs;

	// Create a descriptor of the requested builder work, to generate a TLAS
	m_BuildDesc = {};
	m_BuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	m_BuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	m_BuildDesc.Inputs.InstanceDescs = dynamicDataParams.uploadResource->GetGPUVirtualAddress();
	m_BuildDesc.Inputs.NumDescs = numInstances;
	m_BuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	m_BuildDesc.DestAccelerationStructureData = static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.ScratchAccelerationStructureData = static_cast<D3D12GraphicsBuffer*>(m_pScratchBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.SourceAccelerationStructureData = 0;
}
