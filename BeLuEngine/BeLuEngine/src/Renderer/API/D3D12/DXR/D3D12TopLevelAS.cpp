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

void D3D12TopLevelAS::AddInstance(
	IBottomLevelAS* BLAS,
	const DirectX::XMMATRIX& m_Transform,
	unsigned int hitGroupIndex,
	bool giveShadows)
{
	m_Instances.emplace_back(Instance(static_cast<D3D12BottomLevelAS*>(BLAS)->m_pResultBuffer, m_Transform, m_InstanceCounter++, hitGroupIndex, giveShadows));
	//BL_LOG_INFO("Added instance! %d\n", m_InstanceCounter);
}

void D3D12TopLevelAS::Reset()
{
	m_Instances.clear();
	m_InstanceCounter = 0;
}

void D3D12TopLevelAS::GenerateBuffers()
{
	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	// TODO: fast trace?
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	// Describe the work being requested, in this case the construction of a
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = static_cast<UINT>(m_Instances.size());
	prebuildDesc.Flags = flags;

	// This structure is used to hold the sizes of the required scratch memory and resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	device5->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

	// Buffer sizes need to be 256-byte-aligned
	unsigned int scratchSizeInBytes = (info.ScratchDataSizeInBytes + 255) & ~255;
	unsigned int resultSizeInBytes = (info.ResultDataMaxSizeInBytes + 255) & ~255;

	// Create buffers for scratch and result
	BL_SAFE_DELETE(m_pScratchBuffer);
	BL_SAFE_DELETE(m_pResultBuffer);

	m_pScratchBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::UnorderedAccessBuffer, scratchSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"SCRATCHBUFFER_TLAS");
	m_pResultBuffer = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::RayTracingBuffer, resultSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, L"RESULTBUFFER_TLAS");

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.RaytracingAccelerationStructure.Location = static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->m_pResource->GetGPUVirtualAddress();
}

void D3D12TopLevelAS::SetupAccelerationStructureForBuilding(bool update)
{
	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();

	// Ignore first
	if (m_IsBuilt == false && update == true)
		return;

	//if (update == true)
	//	return;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;// D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	//flags |= update ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

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
	m_BuildDesc.Inputs.Flags = flags;

	m_BuildDesc.DestAccelerationStructureData	 = static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.ScratchAccelerationStructureData = static_cast<D3D12GraphicsBuffer*>(m_pScratchBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.SourceAccelerationStructureData	 = 0;// update ? m_pResult->GetID3D12Resource1()->GetGPUVirtualAddress() : 0;
}

IGraphicsBuffer* D3D12TopLevelAS::GetRayTracingResultBuffer() const
{
	return m_pResultBuffer;
}
