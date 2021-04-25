#include "stdafx.h"
#include "ComputeState.h"

#include "../Misc/Log.h"

#include "../RootSignature.h"
#include "../Shader.h"

ComputeState::ComputeState(ID3D12Device5* device, RootSignature* rootSignature, std::wstring& CSName, std::wstring& psoName)
	:PipelineState(psoName)
{
	m_Cpsd.pRootSignature = rootSignature->GetRootSig();

	m_pCS = createShader(CSName, E_SHADER_TYPE::CS);

	IDxcBlob* csBlob = m_pCS->GetBlob();

	m_Cpsd.CS.pShaderBytecode = csBlob->GetBufferPointer();
	m_Cpsd.CS.BytecodeLength = csBlob->GetBufferSize();

	// Create pipelineStateObject
	HRESULT hr = device->CreateComputePipelineState(&m_Cpsd, IID_PPV_ARGS(&m_pPSO));

	m_pPSO->SetName(psoName.c_str());
	if (!SUCCEEDED(hr))
	{
		BL_LOG_CRITICAL("Failed to create %S\n", psoName);
	}
}

ComputeState::~ComputeState()
{
}

const D3D12_COMPUTE_PIPELINE_STATE_DESC* ComputeState::GetCpsd() const
{
	return &m_Cpsd;
}

Shader* ComputeState::GetShader(E_SHADER_TYPE type) const
{
	if (type == E_SHADER_TYPE::CS)
	{
		return m_pCS;
	}
	else if (type == E_SHADER_TYPE::VS)
	{
		BL_LOG_CRITICAL("There is no vertexShader in \'%S\'\n", m_PsoName);
	}
	else if (type == E_SHADER_TYPE::PS)
	{
		BL_LOG_CRITICAL("There is no pixelShader in \'%S\'\n", m_PsoName);
	}

	return nullptr;
}
