#include "stdafx.h"
#include "PipelineState.h"

#include "../Shader.h"
#include "../Misc/AssetLoader.h"

PipelineState::PipelineState(const std::wstring& psoName)
{
	m_PsoName = psoName;
}

PipelineState::~PipelineState()
{
	SAFE_RELEASE(&m_pPSO);
}

ID3D12PipelineState* PipelineState::GetPSO() const
{
	return m_pPSO;
}

Shader* PipelineState::createShader(const std::wstring& fileName, E_SHADER_TYPE type)
{
	if (type == E_SHADER_TYPE::VS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	else if (type == E_SHADER_TYPE::PS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	else if (type == E_SHADER_TYPE::CS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	return nullptr;
}
