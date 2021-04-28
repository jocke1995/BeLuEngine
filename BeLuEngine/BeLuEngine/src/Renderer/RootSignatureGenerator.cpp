#include "stdafx.h"
#include "RootSignatureGenerator.h"

#include "../Misc/Log.h"

void RootSignatureGenerator::AddDescriptorTableRanges(const std::vector<D3D12_DESCRIPTOR_RANGE>& ranges)
{
	m_DescriptorRanges.push_back(ranges);

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = ranges.size();
	rootParam.DescriptorTable.pDescriptorRanges = m_DescriptorRanges.back().data();

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
	unsigned int sizeOfStruct,
	D3D12_SHADER_VISIBILITY shaderVisibility)
{
	D3D12_ROOT_PARAMETER rootParam		= {};
	rootParam.ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam.Constants.ShaderRegister	= shaderRegister;
	rootParam.Constants.RegisterSpace	= registerSpace;
	rootParam.Constants.Num32BitValues  = sizeOfStruct / sizeof(unsigned int);
	rootParam.ShaderVisibility			= shaderVisibility;

	m_RootParameters.push_back(rootParam);
}

void RootSignatureGenerator::AddStaticSampler(
	D3D12_FILTER filter,
	D3D12_TEXTURE_ADDRESS_MODE adressMode,
	unsigned int shaderRegister, unsigned int registerSpace,
	D3D12_COMPARISON_FUNC comparisonFunc,
	unsigned int maxAnisotropy, // Only if Filter is Anisotropic
	D3D12_STATIC_BORDER_COLOR borderColor) // Only if addressMode is Border
{
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

	// Settings from parameters
	samplerDesc.Filter			= filter;
	samplerDesc.AddressU		= adressMode;
	samplerDesc.AddressV		= adressMode;
	samplerDesc.AddressW		= adressMode;
	samplerDesc.ComparisonFunc	= comparisonFunc;
	samplerDesc.ShaderRegister	= shaderRegister;
	samplerDesc.RegisterSpace	= registerSpace;

	// Default settings
	samplerDesc.MipLODBias = 0;	// offset in mipmap level
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	// Settings that may not be used in this static sampler, different depending on filter and/or adressMode
	samplerDesc.MaxAnisotropy = maxAnisotropy;
	samplerDesc.BorderColor = borderColor;

	m_StaticSamplerDescs.push_back(samplerDesc);
}

ID3D12RootSignature* RootSignatureGenerator::Generate(ID3D12Device* device, bool isLocal)
{
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags			 = isLocal ? D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE : D3D12_ROOT_SIGNATURE_FLAG_NONE;
	rsDesc.NumParameters	 = m_RootParameters.size();
	rsDesc.pParameters		 = m_RootParameters.data();
	rsDesc.NumStaticSamplers = m_StaticSamplerDescs.size();
	rsDesc.pStaticSamplers	 = m_StaticSamplerDescs.data();

#ifdef DEBUG
	unsigned int size = 0;
	for (const D3D12_ROOT_PARAMETER& param : m_RootParameters)
	{
		if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			size += 1;
		}
		else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
		{
			size += param.Constants.Num32BitValues;
		}
		else if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_CBV ||
				 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_UAV || 
				 param.ParameterType == D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_SRV)
		{
			size += 2;
		}
	}
	
	if (size > 64)
	{
		BL_LOG_CRITICAL("RootSignature size: %d DWORDS\n", size);
		DebugBreak();
	}
#endif
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
