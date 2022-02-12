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
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_DELETE(m_pInstanceDescBuffers[i]);
	}

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

bool D3D12TopLevelAS::reAllocateInstanceDescBuffers(unsigned int newSizeInBytes)
{
	BL_ASSERT(newSizeInBytes);

	// Making it larger then it has to, to avoid allocating over and over again if we just add 1 object at a time.
	unsigned int actualNewSizeInBytes = newSizeInBytes * 2;

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		// Delete the old one first
		BL_SAFE_DELETE(m_pInstanceDescBuffers[i]);

		std::wstring bufferName = L"TLAS_InstanceDescBuffer_UploadHeap" + std::to_wstring(i);
		m_pInstanceDescBuffers[i] = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::CPUBuffer, actualNewSizeInBytes, 1, DXGI_FORMAT_UNKNOWN, bufferName);
	}

	m_CurrentMaxInstanceDescSize = actualNewSizeInBytes;

	return true;
}

void D3D12TopLevelAS::MapResultBuffer()
{
	BL_ASSERT(m_pResultBuffer && m_pScratchBuffer);

	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();

	unsigned int numInstances = m_Instances.size();

	unsigned int maxSizeInBytes = numInstances * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
	unsigned int alignedMaxSizeInBytes = (maxSizeInBytes + 255) & ~255; // Align with 256 bytes

	// Check to see if we need new buffers for the sbt
	if (m_CurrentMaxInstanceDescSize < alignedMaxSizeInBytes)
		reAllocateInstanceDescBuffers(alignedMaxSizeInBytes);

	TODO("Don't do this on the heap every frame.. Create a stack allocator!");
	unsigned char* dataToMap = new unsigned char[alignedMaxSizeInBytes];
	memset(dataToMap, 0, alignedMaxSizeInBytes);
	D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(dataToMap);

	// Create the description for each instance
	for (unsigned int i = 0; i < numInstances; i++)
	{
		instanceDescs->InstanceID = m_Instances[i].m_ID;
		instanceDescs->InstanceContributionToHitGroupIndex = m_Instances[i].m_ID;
		instanceDescs->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		DirectX::XMMATRIX m = XMMatrixTranspose(m_Instances[i].m_Transform);
		memcpy(instanceDescs->Transform, &m, sizeof(instanceDescs->Transform));
		instanceDescs->AccelerationStructure = static_cast<D3D12GraphicsBuffer*>(m_Instances[i].m_pBLAS)->m_pResource->GetGPUVirtualAddress();

		// Objects who doesn't give shadows doesn't get this maskFlag added to them.
		if (m_Instances[i].m_GiveShadows)
		{
			instanceDescs->InstanceMask = INSTANCE_MASK_SCENE_MINUS_NOSHADOWOBJECTS;
		}
		instanceDescs->InstanceMask |= INSTANCE_MASK_ENTIRE_SCENE;

		// Move to next object
		instanceDescs++;
	}

	// Set the data into the CPU Buffer
	unsigned int backBufferIndex = D3D12GraphicsManager::GetInstance()->GetCommandInterfaceIndex();
	m_pInstanceDescBuffers[backBufferIndex]->SetData(alignedMaxSizeInBytes, dataToMap);

	// Now it's safe to delete the data since it's mapped in the resource
	delete[] dataToMap;

	// Create a descriptor of the requested builder work, to generate a TLAS
	m_BuildDesc = {};
	m_BuildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	m_BuildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	m_BuildDesc.Inputs.InstanceDescs = static_cast<D3D12GraphicsBuffer*>(m_pInstanceDescBuffers[backBufferIndex])->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.Inputs.NumDescs = numInstances;
	m_BuildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;

	m_BuildDesc.DestAccelerationStructureData = static_cast<D3D12GraphicsBuffer*>(m_pResultBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.ScratchAccelerationStructureData = static_cast<D3D12GraphicsBuffer*>(m_pScratchBuffer)->m_pResource->GetGPUVirtualAddress();
	m_BuildDesc.SourceAccelerationStructureData = 0;
}
