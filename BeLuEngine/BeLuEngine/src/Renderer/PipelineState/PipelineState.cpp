#include "stdafx.h"
#include "PipelineState.h"

#include "../Shader.h"
#include "../Misc/AssetLoader.h"

#include "../API/D3D12/D3D12GraphicsManager.h"

PipelineState::PipelineState(const std::wstring& psoName)
{
	m_PsoName = psoName + L"_PSO";
}

PipelineState::~PipelineState()
{
	D3D12GraphicsManager* graphicsManager = D3D12GraphicsManager::GetInstance();

	BL_ASSERT(m_pPSO);
	graphicsManager->AddD3D12ObjectToDefferedDeletion(m_pPSO);
}

ID3D12PipelineState* PipelineState::GetPSO() const
{
	return m_pPSO;
}

Shader* PipelineState::createShader(const std::wstring& fileName, E_SHADER_TYPE type)
{
	if (type == E_SHADER_TYPE::VS ||
		type == E_SHADER_TYPE::PS ||
		type == E_SHADER_TYPE::CS)
	{
		return AssetLoader::Get()->loadShader(fileName, type);
	}
	else
	{
		BL_ASSERT_MESSAGE(false, "Invalid E_SHADER_TYPE when creating pipeline state");
	}

	return nullptr;
}
