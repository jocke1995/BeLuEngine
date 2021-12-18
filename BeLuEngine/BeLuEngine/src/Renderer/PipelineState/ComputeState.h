#ifndef COMPUTESTATE_H
#define COMPUTESTATE_H

#include "PipelineState.h"

class Shader;

class ComputeState : public PipelineState
{
public:
	ComputeState(std::wstring CSName, std::wstring psoName);

	virtual ~ComputeState();

	const D3D12_COMPUTE_PIPELINE_STATE_DESC* GetCpsd() const;
	Shader* GetShader(E_SHADER_TYPE type = E_SHADER_TYPE::CS) const override;
private:
	Shader* m_pCS = nullptr;
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_Cpsd = {};
};

#endif 