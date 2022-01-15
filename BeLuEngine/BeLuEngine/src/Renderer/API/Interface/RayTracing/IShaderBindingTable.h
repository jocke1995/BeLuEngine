#ifndef ISHADERBINDINGTABLE_H
#define ISHADERBINDINGTABLE_H

#include "RenderCore.h"

class IGraphicsBuffer;
class IRayTracingPipelineState;

struct ShaderRecord
{
	ShaderRecord(const std::wstring entryPointName, const std::vector<IGraphicsBuffer*> inputData);

	std::wstring m_EntryPointName;
	std::vector<IGraphicsBuffer*> m_InputData;
};

enum class E_SHADER_RECORD_TYPE
{
	RAY_GENERATION_SHADER_RECORD,
	MISS_SHADER_RECORD,
	HIT_GROUP_SHADER_RECORD,
	NUM_SHADER_RECORD_TYPES
};

class IShaderBindingTable
{
public:
	virtual ~IShaderBindingTable();
	static IShaderBindingTable* Create(const std::wstring& name);

	// Call on each update, before adding shaderRecords 
	void Reset();

	// Optional: Call to avoid unnecessary memory allocations
	//void PreAllocateShaderRecordMemory(E_SHADER_RECORD_TYPE shaderRecordType, unsigned int numElements);

	void AddShaderRecord(E_SHADER_RECORD_TYPE shaderRecordType, const std::wstring& entryPoint, const std::vector<IGraphicsBuffer*>& inputData);

	virtual void GenerateShaderBindingTable(IRayTracingPipelineState* rtPipelineState) = 0;

	unsigned int GetRayGenSectionSize();
	unsigned int GetMissSectionSize();
	unsigned int GetHitGroupSectionSize();

	unsigned int GetMaxRayGenShaderRecordSize();
	unsigned int GetMaxMissShaderRecordSize();
	unsigned int GetMaxHitGroupShaderRecordSize();
protected:

	std::vector<ShaderRecord> m_RayGenerationRecords;
	std::vector<ShaderRecord> m_MissRecords;
	std::vector<ShaderRecord> m_HitGroupRecords;

	unsigned int mMaxRayGenerationSize = 0;
	unsigned int mMaxMissRecordSize = 0;
	unsigned int mMaxHitGroupSize = 0;
#ifdef DEBUG
	std::wstring m_DebugName = L"";
#endif
};

#endif