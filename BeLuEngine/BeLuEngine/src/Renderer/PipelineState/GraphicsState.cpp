#include "stdafx.h"
#include "GraphicsState.h"

#include "../Misc/Log.h"

#include "../Shader.h"

#include "../API/D3D12/D3D12GraphicsManager.h"

GraphicsState::GraphicsState(const std::wstring& VSName, const std::wstring& PSName, D3D12_GRAPHICS_PIPELINE_STATE_DESC* gpsd, const std::wstring& psoName)
	:PipelineState(psoName)
{
	D3D12GraphicsManager* manager = D3D12GraphicsManager::GetInstance();
	ID3D12Device5* device5 = manager->GetDevice();

	// Set the rootSignature in the pipeline state object descriptor
	m_pGPSD = gpsd;

	m_pGPSD->pRootSignature = static_cast<D3D12GraphicsManager*>(IGraphicsManager::GetBaseInstance())->m_pGlobalRootSig;

	m_pVS = createShader(VSName, E_SHADER_TYPE::VS);
	m_pPS = createShader(PSName, E_SHADER_TYPE::PS);

	IDxcBlob* vsBlob = m_pVS->GetBlob();
	IDxcBlob* psBlob = m_pPS->GetBlob();

	m_pGPSD->VS.pShaderBytecode = vsBlob->GetBufferPointer();
	m_pGPSD->VS.BytecodeLength = vsBlob->GetBufferSize();
	m_pGPSD->PS.pShaderBytecode = psBlob->GetBufferPointer();
	m_pGPSD->PS.BytecodeLength = psBlob->GetBufferSize();

	// Create pipelineStateObject
	HRESULT hr = device5->CreateGraphicsPipelineState(m_pGPSD, IID_PPV_ARGS(&m_pPSO));

	if (FAILED(hr))
	{
		BL_LOG_CRITICAL("Failed to create %S\n", psoName);
	}
	m_pPSO->SetName(psoName.c_str());
}

GraphicsState::~GraphicsState()
{
}

const D3D12_GRAPHICS_PIPELINE_STATE_DESC* GraphicsState::GetGpsd() const
{
	return m_pGPSD;
}

Shader* GraphicsState::GetShader(E_SHADER_TYPE type) const
{
	if (type == E_SHADER_TYPE::VS)
	{
		return m_pVS;
	}
	else if (type == E_SHADER_TYPE::PS)
	{
		return m_pPS;
	}
	
	BL_LOG_CRITICAL("There is no ComputeShader in \'%S\'\n", m_PsoName);
	return nullptr;
}
