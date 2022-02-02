#include "stdafx.h"
#include "D3D12ShaderBindingTable.h"

#include "../D3D12GraphicsManager.h"
#include "../D3D12GraphicsBuffer.h"

#include "D3D12RayTracingPipelineState.h"

D3D12ShaderBindingTable::D3D12ShaderBindingTable(const std::wstring& name)
{
#ifdef DEBUG
	m_DebugName = name;
#endif
}

D3D12ShaderBindingTable::~D3D12ShaderBindingTable()
{
	for (unsigned int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		BL_SAFE_DELETE(m_pSBTBuffer[i]);
	}
}

void D3D12ShaderBindingTable::GenerateShaderBindingTable(IRayTracingPipelineState* rtPipelineState)
{
	BL_ASSERT(rtPipelineState);

	D3D12RayTracingPipelineState* d3d12RayTracingState = static_cast<D3D12RayTracingPipelineState*>(rtPipelineState);
	D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();

	unsigned int sbtSize = calculateShaderBindingTableSize();

	// Check to see if we need new buffers for the sbt
	if (m_CurrentMaxSBTSize < sbtSize)
		reAllocateHeaps(sbtSize);

	// Map the ShaderBindingTable buffer
	TODO("Don't do this on the heap every frame.. Create a stack allocator!");
	unsigned char* dataToMap = new unsigned char[sbtSize];
	memset(dataToMap, 0, sbtSize);
	unsigned char* pDataToMap = dataToMap;

	unsigned int offset = 0;
	offset = copyShaderRecords(d3d12RayTracingState, pDataToMap, m_RayGenerationRecords, mMaxRayGenerationSize);
	pDataToMap += offset;

	offset = copyShaderRecords(d3d12RayTracingState, pDataToMap, m_MissRecords, mMaxMissRecordSize);
	pDataToMap += offset;

	offset = copyShaderRecords(d3d12RayTracingState, pDataToMap, m_HitGroupRecords, mMaxHitGroupSize);

	// Set the data into the CPU Buffer
	unsigned int backBufferIndex = d3d12Manager->GetCommandInterfaceIndex();
	m_pSBTBuffer[backBufferIndex]->SetData(sbtSize, dataToMap);

	// Data is now free to release
	delete[] dataToMap;
}

unsigned int D3D12ShaderBindingTable::calculateShaderBindingTableSize()
{
	auto calculateMaxShaderRecordSize = [](const std::vector<ShaderRecord> shaderRecords) -> unsigned int
	{
		// Find the maximum number of parameters used by a single entry
		unsigned int maxArgs = 0;
		for (const ShaderRecord& shaderRecord : shaderRecords)
		{
			maxArgs = Max(maxArgs, (unsigned int)shaderRecord.m_InputData.size());
		}

		// A SBT entry is made of a program ID and a set of parameters, taking 8 bytes each. Those
		// parameters can either be 8-bytes pointers, or 4-bytes constants
		unsigned int entrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * maxArgs;

		// Align
		unsigned int entrySizePadded = (entrySize + (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT - 1)) & ~(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT - 1);

		return entrySizePadded;
	};

	// Compute the entry size of each program type depending on the maximum number of parameters in each category
	mMaxRayGenerationSize	= calculateMaxShaderRecordSize(m_RayGenerationRecords);
	mMaxMissRecordSize		= calculateMaxShaderRecordSize(m_MissRecords);
	mMaxHitGroupSize		= calculateMaxShaderRecordSize(m_HitGroupRecords);

	// The total SBT size is the sum of the entries for ray generation, miss and hit groups
	unsigned int sbtSizeUnPadded =
		mMaxRayGenerationSize * m_RayGenerationRecords.size() +	// Raygen Sizes
		mMaxMissRecordSize * m_MissRecords.size() +				// Miss Sizes
		mMaxHitGroupSize * m_HitGroupRecords.size();			// Hitgroup Sizes
	;
	TODO("Wrong byte alignment? Maybe should be (256 - 1)?")
	unsigned int sbtSizePadded = (sbtSizeUnPadded + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) & ~D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

	return sbtSizePadded;
}

bool D3D12ShaderBindingTable::reAllocateHeaps(unsigned int sizeInBytes)
{
	BL_ASSERT(sizeInBytes);

	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = graphicsManager->GetDevice();

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		// Delete the old one first
		BL_SAFE_DELETE(m_pSBTBuffer[i]);

		std::wstring bufferName = L"ShaderBindingTable_UploadHeap" + std::to_wstring(i);
		m_pSBTBuffer[i] = IGraphicsBuffer::Create(E_GRAPHICSBUFFER_TYPE::CPUBuffer, sizeInBytes, 1, DXGI_FORMAT_UNKNOWN, bufferName);
	}

	m_CurrentMaxSBTSize = sizeInBytes;

	return true;
}

unsigned int D3D12ShaderBindingTable::copyShaderRecords(
	D3D12RayTracingPipelineState* d3d12RayTracingPipelineState,
	unsigned char* pMappedData,
	const std::vector<ShaderRecord>& shaderRecords,
	unsigned int maxShaderRecordSize)
{
	ID3D12StateObjectProperties* stateObjectProps = d3d12RayTracingPipelineState->m_pRTStateObjectProps;

	uint8_t* pData = pMappedData;
	for (const ShaderRecord& shaderRecord : shaderRecords)
	{
		unsigned int currentOffset = 0;
		// Get the shader identifier
		void* shaderIdentifier = stateObjectProps->GetShaderIdentifier(shaderRecord.m_EntryPointName.c_str());
		BL_ASSERT(shaderIdentifier);

		// Copy the shader identifier
		memcpy(pData, shaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		currentOffset += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

		// Copy all its descriptors
		unsigned int descriptorPointerSize = sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
		for (unsigned int i = 0; i < shaderRecord.m_InputData.size(); i++)
		{
			D3D12GraphicsBuffer* d3d12Buffer = static_cast<D3D12GraphicsBuffer*>(shaderRecord.m_InputData[i]);
			D3D12_GPU_VIRTUAL_ADDRESS vAddr = d3d12Buffer->m_pResource->GetGPUVirtualAddress();
			
			memcpy(pData + currentOffset, reinterpret_cast<void*>(&vAddr), descriptorPointerSize);

			currentOffset += descriptorPointerSize;
		}

		// Offset the pointer for the next ShaderRecord
		pData += maxShaderRecordSize;
	}

	// Return the number of bytes actually written to the output buffer so that the next section knows were to begin
	return static_cast<unsigned int>(shaderRecords.size()) * maxShaderRecordSize;
}
