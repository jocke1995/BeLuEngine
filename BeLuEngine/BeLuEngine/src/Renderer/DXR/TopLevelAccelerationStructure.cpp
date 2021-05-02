#include "stdafx.h"
#include "TopLevelAccelerationStructure .h"

#include "../Misc/Log.h"

#include "../GPUMemory/Resource.h"
TopLevelAccelerationStructure::TopLevelAccelerationStructure()
{
}

TopLevelAccelerationStructure::~TopLevelAccelerationStructure()
{
	SAFE_DELETE(m_pInstanceDesc);
}

void TopLevelAccelerationStructure::AddInstance(
	Resource* BLAS,
	const DirectX::XMMATRIX& m_Transform,
	unsigned int instanceID,
	unsigned int hitGroupIndex)
{
	m_Instances.emplace_back(Instance(BLAS, m_Transform, instanceID, hitGroupIndex));
}

void TopLevelAccelerationStructure::Reset()
{
	m_Instances.clear();
}

void TopLevelAccelerationStructure::GenerateBuffers(ID3D12Device5* pDevice)
{
	// TODO: fast trace?
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	// Describe the work being requested, in this case the construction of a
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS prebuildDesc = {};
	prebuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	prebuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	prebuildDesc.NumDescs = static_cast<UINT>(m_Instances.size());
	prebuildDesc.Flags = flags;

	// This structure is used to hold the sizes of the required scratch memory and
	// resulting AS
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};

	pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&prebuildDesc, &info);

	// Buffer sizes need to be 256-byte-aligned
	unsigned int scratchSizeInBytes = (info.ScratchDataSizeInBytes + 255) & ~255;
	unsigned int resultSizeInBytes = (info.ResultDataMaxSizeInBytes + 255) & ~255;
	m_InstanceDescsSizeInBytes = (sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * static_cast<UINT64>(m_Instances.size() + 255)) & ~255;


	// Create buffers for scratch, result and instance
	SAFE_DELETE(m_pScratch);
	SAFE_DELETE(m_pResult);
	SAFE_DELETE(m_pInstanceDesc);

	m_pScratch = new Resource(
		pDevice, scratchSizeInBytes,
		RESOURCE_TYPE::DEFAULT, L"scratchTopLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	D3D12_RESOURCE_STATES stateRAS = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	m_pResult = new Resource(
		pDevice, resultSizeInBytes,
		RESOURCE_TYPE::DEFAULT, L"resultTopLevel",
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		&stateRAS);

	stateRAS = D3D12_RESOURCE_STATE_GENERIC_READ;
	m_pInstanceDesc = new Resource(
		pDevice, m_InstanceDescsSizeInBytes,
		RESOURCE_TYPE::UPLOAD, L"instanceDescTopLevel",
		D3D12_RESOURCE_FLAG_NONE,
		&stateRAS);
}

void TopLevelAccelerationStructure::SetupAccelerationStructureForBuilding(ID3D12Device5* pDevice, bool update)
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	flags |= update ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	// Copy the descriptors in the target descriptor buffer
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
	m_pInstanceDesc->GetID3D12Resource1()->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));

#ifdef DEBUG
	if (instanceDescs == nullptr)
	{
		BL_LOG_CRITICAL("Failed to Map into the TLAS.\n");
	}
#endif

	// Initialize the memory to zero on the first time only
	if (update == false)
	{
		ZeroMemory(instanceDescs, m_InstanceDescsSizeInBytes);
	}

	unsigned int numInstances = m_Instances.size();
	// Create the description for each instance
	for (unsigned int i = 0; i < numInstances; i++)
	{
		instanceDescs[i].InstanceID = m_Instances[i].m_ID;
		instanceDescs[i].InstanceContributionToHitGroupIndex = m_Instances[i].m_HitGroupIndex;
		instanceDescs[i].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		DirectX::XMMATRIX m = XMMatrixTranspose(m_Instances[i].m_Transform); // GLM is column major, the INSTANCE_DESC is row major
		memcpy(instanceDescs[i].Transform, &m, sizeof(instanceDescs[i].Transform));
		instanceDescs[i].AccelerationStructure = m_Instances[i].m_pBLAS->GetID3D12Resource1()->GetGPUVirtualAddress();

		// TODO: Add the possibilty to set flags for enabling shadows etc..
		instanceDescs[i].InstanceMask = 0xFF;
	}

	m_pInstanceDesc->GetID3D12Resource1()->Unmap(0, nullptr);

	// If the TLAS is to be updated. Give it the old TLAS as source
	D3D12_GPU_VIRTUAL_ADDRESS pSourceAS = update ? m_pInstanceDesc->GetID3D12Resource1()->GetGPUVirtualAddress() : 0;

	// Create a descriptor of the requested builder work, to generate a TLAS
	m_BuildDesc = {};
	m_BuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	m_BuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	m_BuildDesc.Inputs.InstanceDescs = m_pInstanceDesc->GetID3D12Resource1()->GetGPUVirtualAddress();
	m_BuildDesc.Inputs.NumDescs = numInstances;
	m_BuildDesc.Inputs.Flags = flags;

	m_BuildDesc.DestAccelerationStructureData = { m_pResult->GetID3D12Resource1()->GetGPUVirtualAddress() };
	m_BuildDesc.ScratchAccelerationStructureData = { m_pScratch->GetID3D12Resource1()->GetGPUVirtualAddress()};
	m_BuildDesc.SourceAccelerationStructureData = pSourceAS;
}
