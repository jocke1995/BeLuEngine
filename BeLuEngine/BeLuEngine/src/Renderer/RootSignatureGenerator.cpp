#include "stdafx.h"
#include "RootSignatureGenerator.h"

#include "../Misc/Log.h"

void RootSignatureGenerator::AddDescriptorTableRanges(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges)
{
	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = ranges.size();
	rootParam.DescriptorTable.pDescriptorRanges = ranges.data();

	m_RootParameters.push_back(rootParam);
}

void RootSignatureGenerator::AddRootDescriptor(
	D3D12_ROOT_PARAMETER_TYPE descriptorType,
	unsigned int shaderRegister,
	unsigned int registerSpace,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_ROOT_PARAMETER rootParam		= {};
	rootParam.ParameterType				= descriptorType;
	rootParam.Descriptor.ShaderRegister = shaderRegister;
	rootParam.Descriptor.RegisterSpace	= registerSpace;
	rootParam.ShaderVisibility			= shaderVisibility;

	m_RootParameters.push_back(rootParam);
}

void RootSignatureGenerator::AddRootConstant(
	unsigned int shaderRegister,
	unsigned int registerSpace,
	unsigned int numRootConstants,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_ROOT_PARAMETER rootParam		= {};
	rootParam.ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam.Constants.ShaderRegister	= shaderRegister;
	rootParam.Constants.RegisterSpace	= registerSpace;
	rootParam.Constants.Num32BitValues  = numRootConstants;
	rootParam.ShaderVisibility			= shaderVisibility;

	m_RootParameters.push_back(rootParam);
}

ID3D12RootSignature* RootSignatureGenerator::Generate(ID3D12Device* device, bool isLocal)
{
	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
	rsDesc.NumParameters = m_RootParameters.size();
	rsDesc.pParameters = m_RootParameters.data();
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers = nullptr; // TODO

	ID3DBlob* errorMessages = nullptr;
	ID3DBlob* pBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&pBlob,
		&errorMessages);

	if (FAILED(hr) && errorMessages)
	{
		BL_LOG_CRITICAL("Failed to Serialize RootSignature\n");

		const char* errorMsg = static_cast<const char*>(errorMessages->GetBufferPointer());
		BL_LOG_CRITICAL("%s\n", errorMsg);
	}

	ID3D12RootSignature* pRootSig;
	hr = device->CreateRootSignature(
		0,
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		IID_PPV_ARGS(&pRootSig));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to create RootSignature\n");
	}
	return pRootSig;
}
