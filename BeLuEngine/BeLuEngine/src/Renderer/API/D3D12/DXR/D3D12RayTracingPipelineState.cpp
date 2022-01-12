#include "stdafx.h"
#include "D3D12RayTracingPipelineState.h"

#include "../D3D12GraphicsManager.h"
#include "../Renderer/Shaders/Shader.h"

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState(const RayTracingPSDesc& desc, const std::wstring& name)
{
	D3D12_STATE_SUBOBJECT stateObjects[100]{};
	UINT numSubObjects = 0;
	auto nextSuboject = [&numSubObjects, &stateObjects]() -> D3D12_STATE_SUBOBJECT*
	{
		D3D12_STATE_SUBOBJECT* nextSubObject = stateObjects + numSubObjects++;
		return nextSubObject;
	};

#pragma region Libraries

	D3D12_STATE_SUBOBJECT* dxilSubObject = nullptr;
	// Add all the DXIL libraries
	unsigned int numShaders = desc.m_RayTracingShaders.size();
	std::vector<D3D12_DXIL_LIBRARY_DESC> dxilLibDescs(numShaders);
	for (unsigned int i = 0; i < numShaders; i++)
	{
		// For all Entrypoint sharing the same shader file
		unsigned int numEntryPoints = desc.m_RayTracingShaders[i].m_ShaderEntryPointNames.size();
		std::vector<D3D12_EXPORT_DESC> exportDescs(numEntryPoints);
		for (unsigned int j = 0; j < numEntryPoints; j++)
		{
			exportDescs[j].Name = desc.m_RayTracingShaders[i].m_ShaderEntryPointNames[j].c_str();
			exportDescs[j].ExportToRename = nullptr;
			exportDescs[j].Flags = D3D12_EXPORT_FLAG_NONE;
		}

		dxilLibDescs[i].DXILLibrary.pShaderBytecode = desc.m_RayTracingShaders[i].m_Shader->GetBlob()->GetBufferPointer();
		dxilLibDescs[i].DXILLibrary.BytecodeLength = desc.m_RayTracingShaders[i].m_Shader->GetBlob()->GetBufferSize();
		dxilLibDescs[i].pExports = exportDescs.data();
		dxilLibDescs[i].NumExports = numEntryPoints;
			
		dxilSubObject = nextSuboject();
		dxilSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		dxilSubObject->pDesc = &dxilLibDescs[i];

	}
#pragma endregion

#pragma region HitGroups
	D3D12_STATE_SUBOBJECT* hitGroupSubObject = nullptr;
	unsigned int numHitGroups = desc.m_HitGroups.size();
	std::vector<D3D12_HIT_GROUP_DESC> hitGroupDescs(numHitGroups);
	for (unsigned int i = 0; i < numHitGroups; i++)
	{
		TODO("Support for other types");
		hitGroupDescs[i].Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		hitGroupDescs[i].HitGroupExport				= desc.m_HitGroups[i].m_HitGroupName.c_str();
		hitGroupDescs[i].ClosestHitShaderImport		= desc.m_HitGroups[i].m_ClosestHitName.empty()	? nullptr : desc.m_HitGroups[i].m_ClosestHitName.c_str();
		hitGroupDescs[i].AnyHitShaderImport			= desc.m_HitGroups[i].m_AnyHitName.empty()		? nullptr : desc.m_HitGroups[i].m_AnyHitName.c_str();
		hitGroupDescs[i].IntersectionShaderImport	= desc.m_HitGroups[i].m_IntersectionName.empty()? nullptr : desc.m_HitGroups[i].m_IntersectionName.c_str();
		
		hitGroupSubObject = nextSuboject();
		hitGroupSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hitGroupSubObject->pDesc = &hitGroupDescs[i];
	}
#pragma endregion

#pragma region Misc
	// Add a subobject for the shader payload configuration
	D3D12_RAYTRACING_SHADER_CONFIG rtShaderConfig = {};
	rtShaderConfig.MaxPayloadSizeInBytes	= desc.m_PayloadSize;
	rtShaderConfig.MaxAttributeSizeInBytes	= desc.m_MaxAttributeSize;

	D3D12_STATE_SUBOBJECT* rtShaderConfigSubObject = nextSuboject();
	rtShaderConfigSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	rtShaderConfigSubObject->pDesc = &rtShaderConfig;
#pragma endregion

#pragma region CreateRootSignaturesFromParameters
#pragma endregion
}

D3D12RayTracingPipelineState::~D3D12RayTracingPipelineState()
{
}

ID3D12RootSignature* D3D12RayTracingPipelineState::createLocalRootSignature(const std::vector<IRayTracingRootSignatureParams>& rootSigParams)
{
	auto createLocalRootSig = [](const D3D12_ROOT_SIGNATURE_DESC& rootDesc) -> ID3D12RootSignature*
	{
		BL_ASSERT(rootDesc.Flags & D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

		D3D12GraphicsManager* d3d12Manager = D3D12GraphicsManager::GetInstance();
		ID3D12Device5* device5 = d3d12Manager->GetDevice();

		ID3DBlob* errorMessages = nullptr;
		ID3DBlob* m_pBlob = nullptr;

		HRESULT hr = D3D12SerializeRootSignature(
			&rootDesc,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&m_pBlob,
			&errorMessages);

		if (!D3D12GraphicsManager::CHECK_HRESULT(hr) && errorMessages)
		{
			BL_LOG_CRITICAL("Failed to Serialize local RootSignature\n");

			const char* errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
			BL_LOG_CRITICAL("%s\n", errorMsg);
		}

		ID3D12RootSignature* pRootSig;
		hr = device5->CreateRootSignature(
			0,
			m_pBlob->GetBufferPointer(),
			m_pBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSig));

		if (!D3D12GraphicsManager::CHECK_HRESULT(hr))
		{
			BL_ASSERT_MESSAGE(pRootSig != nullptr, "Failed to create local RootSignature\n");
		}

		BL_SAFE_RELEASE(&m_pBlob);
		return pRootSig;
	};

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 0;
	rootDesc.pParameters = nullptr;
	rootDesc.NumStaticSamplers = 0;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	rootDesc.pStaticSamplers = nullptr;

	// Return empty rootSignature if no params were submitted
	if(rootSigParams.size() == 0)
		return createLocalRootSig(rootDesc);

	// Note: Only RootParams are supported atm, no 32BitConstants or DescriptorTables
	unsigned int numRootParams = rootSigParams.size();
	std::vector<D3D12_ROOT_PARAMETER> rootParams(numRootParams);
	for (unsigned int i = 0; i < numRootParams; i++)
	{
		rootParams[i].ParameterType = ConvertBLRootParameterTypeToD3D12RootParameterType(rootSigParams[i].rootParamType);
		rootParams[i].Descriptor.ShaderRegister = rootSigParams[i].shaderRegister;
		rootParams[i].Descriptor.RegisterSpace	= rootSigParams[i].registerSpace;
		rootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	rootDesc.NumParameters = numRootParams;
	rootDesc.pParameters = rootParams.data();

	return createLocalRootSig(rootDesc);
}
