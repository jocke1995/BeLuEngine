#include "stdafx.h"
#include "D3D12RayTracingPipelineState.h"

#include "../D3D12GraphicsManager.h"
#include "../Renderer/Shaders/Shader.h"

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState(const RayTracingPSDesc& desc, const std::wstring& name)
{
	D3D12GraphicsManager* d3d12Manager = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance());
	const unsigned int maxSubObjects = 100;
	D3D12_STATE_SUBOBJECT stateObjects[maxSubObjects]{};
	UINT numSubObjects = 0;
	auto nextSubobject = [&numSubObjects, &stateObjects, maxSubObjects]() -> D3D12_STATE_SUBOBJECT*
	{
		BL_ASSERT(numSubObjects != maxSubObjects - 1);

		return stateObjects + numSubObjects++;
	};

#pragma region Libraries

	D3D12_STATE_SUBOBJECT* dxilSubObject = nullptr;
	// Add all the DXIL libraries
	unsigned int numShaders = desc.m_RayTracingShaders.size();
	std::vector<D3D12_DXIL_LIBRARY_DESC> dxilLibDescs(numShaders);
	std::vector<D3D12_EXPORT_DESC> exportDescs(numShaders);	// Currently only supporting 1 entryPoint per shaderFile.. this is something that could be expanded..

	for (unsigned int i = 0; i < numShaders; i++)
	{
		// For all Entrypoint sharing the same shader file
		exportDescs[i].Name = desc.m_RayTracingShaders[i].m_ShaderEntryPointNames.c_str();
		exportDescs[i].ExportToRename = nullptr;
		exportDescs[i].Flags = D3D12_EXPORT_FLAG_NONE;

		dxilLibDescs[i].DXILLibrary.pShaderBytecode = desc.m_RayTracingShaders[i].m_Shader->GetBlob()->GetBufferPointer();
		dxilLibDescs[i].DXILLibrary.BytecodeLength = desc.m_RayTracingShaders[i].m_Shader->GetBlob()->GetBufferSize();
		dxilLibDescs[i].pExports = &exportDescs[i];
		dxilLibDescs[i].NumExports = 1;
			
		dxilSubObject = nextSubobject();
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
		
		hitGroupSubObject = nextSubobject();
		hitGroupSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hitGroupSubObject->pDesc = &hitGroupDescs[i];
	}
#pragma endregion

#pragma region ShaderConfiguration
	// Add a subobject for the shader configuration
	D3D12_RAYTRACING_SHADER_CONFIG rtShaderConfig = {};
	rtShaderConfig.MaxPayloadSizeInBytes	= desc.m_PayloadSize;
	rtShaderConfig.MaxAttributeSizeInBytes	= desc.m_MaxAttributeSize;

	D3D12_STATE_SUBOBJECT* rtShaderConfigSubObject = nextSubobject();
	rtShaderConfigSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	rtShaderConfigSubObject->pDesc = &rtShaderConfig;

	// Setup exports/Names. 
	// This part stores the name of each Raygen, Miss and/or Hitgroups
	// These names are the same names that will be associated with their corresponding rootSignatures
	unsigned int numExports = desc.m_RootSignatureAssociations.size();
	std::vector<const wchar_t*> exportNames(numExports);
	for (unsigned int i = 0; i < numExports; i++)
	{
		exportNames[i] = desc.m_RootSignatureAssociations[i].m_EntryPointNameOrHitGroupName.c_str();
	}

	const wchar_t** ppShaderExports = exportNames.data();
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.NumExports = numExports;
	shaderPayloadAssociation.pExports	= ppShaderExports;
	shaderPayloadAssociation.pSubobjectToAssociate = rtShaderConfigSubObject;

	D3D12_STATE_SUBOBJECT* shaderPayloadAssociationObject = nextSubobject();
	shaderPayloadAssociationObject->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderPayloadAssociationObject->pDesc = &shaderPayloadAssociation;
#pragma endregion

#pragma region AssociateRootSignatures
	D3D12_STATE_SUBOBJECT* rootSigObject = nullptr;
	D3D12_STATE_SUBOBJECT* rootSigAssociationObject = nullptr;

	unsigned int numRootSignatureAssociations = desc.m_RootSignatureAssociations.size();
	std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> rootSigAssociation(numRootSignatureAssociations);
	std::vector<LPCWSTR> shaderExportsVec(numRootSignatureAssociations);
	std::vector<ID3D12RootSignature*> rootSigs(numRootSignatureAssociations);

	for (unsigned int i = 0; i < numRootSignatureAssociations; i++)
	{
		rootSigObject = nextSubobject();
		rootSigs[i] = createLocalRootSignature(desc.m_RootSignatureAssociations[i]);
		rootSigObject->Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		rootSigObject->pDesc = &rootSigs[i];

		shaderExportsVec[i] = { desc.m_RootSignatureAssociations[i].m_EntryPointNameOrHitGroupName.c_str() };
		
		rootSigAssociation[i].NumExports = 1;
		rootSigAssociation[i].pExports = &shaderExportsVec[i];
		rootSigAssociation[i].pSubobjectToAssociate = rootSigObject;

		rootSigAssociationObject = nextSubobject();
		rootSigAssociationObject->Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		rootSigAssociationObject->pDesc = &rootSigAssociation[i];
	}
#pragma endregion

#pragma region GlobalRootSignature
	ID3D12RootSignature* globalRootSigParam = d3d12Manager->m_pGlobalRootSig;

	D3D12_STATE_SUBOBJECT* globalRootSigSubObject = nextSubobject();
	globalRootSigSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSigSubObject->pDesc = &globalRootSigParam;
#pragma endregion

#pragma region PipelineConfiguration
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = desc.m_MaxRecursionDepth;

	D3D12_STATE_SUBOBJECT* pipelineConfigSubObject = nextSubobject();
	pipelineConfigSubObject->Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigSubObject->pDesc = &pipelineConfig;
#pragma endregion

#pragma region Finalize
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = numSubObjects;
	pipelineDesc.pSubobjects = stateObjects;

	ID3D12Device5* device5 = d3d12Manager->m_pDevice5;
	HRESULT hr = device5->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&m_pRayTracingStateObject));

	d3d12Manager->CHECK_HRESULT(hr);

	m_pRayTracingStateObject->SetName(name.c_str());
#pragma endregion
}

D3D12RayTracingPipelineState::~D3D12RayTracingPipelineState()
{
	// Release RootSignatures
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();
	for (ID3D12RootSignature* rootSig : m_RootSigs)
	{
		graphicsManager->AddD3D12ObjectToDefferedDeletion(rootSig);
	}

	// StateObject
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pRayTracingStateObject);
}

ID3D12RootSignature* D3D12RayTracingPipelineState::createLocalRootSignature(const RayTracingRootSignatureAssociation& rootSigAssociation)
{
	auto createLocalRootSig = [this, rootSigAssociation](const D3D12_ROOT_SIGNATURE_DESC& rootDesc) -> ID3D12RootSignature*
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

		D3D12GraphicsManager::CHECK_HRESULT(pRootSig->SetName(rootSigAssociation.m_RootSigDebugName.c_str()));

		BL_SAFE_RELEASE(&m_pBlob);

		m_RootSigs.push_back(pRootSig);	// Hacky way to be able to free them when deleting the state...
		return pRootSig;
	};

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = 0;
	rootDesc.pParameters = nullptr;
	rootDesc.NumStaticSamplers = 0;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	rootDesc.pStaticSamplers = nullptr;

	const std::vector<IRayTracingRootSignatureParams>& rootSigParams = rootSigAssociation.m_RootSigParams;
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
