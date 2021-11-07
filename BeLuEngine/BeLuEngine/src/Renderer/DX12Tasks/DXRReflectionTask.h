#ifndef DXREFLECTIONTASK_H
#define DXREFLECTIONTASK_H

#include "DXRTASk.h"

class Shader;

class DXRReflectionTask : public DXRTask
{
public:
	DXRReflectionTask(ID3D12Device5* device,
		ID3D12RootSignature* globalRootSignature,
		const std::wstring& rayGenShaderName, const std::wstring& ClosestHitShaderName, const std::wstring& missShaderName,
		ID3D12StateObject* rayTracingStateObject,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~DXRReflectionTask();

	void Execute() override final;

private:
	Shader* m_pRayGenShader = nullptr;
	Shader* m_pHitShader = nullptr;
	Shader* m_pMissShader = nullptr;

	ID3D12RootSignature* m_pRayGenSignature;
	ID3D12RootSignature* m_pHitSignature;
	ID3D12RootSignature* m_pMissSignature;
};

#endif