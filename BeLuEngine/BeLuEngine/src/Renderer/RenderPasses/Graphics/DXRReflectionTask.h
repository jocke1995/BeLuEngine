#ifndef DXREFLECTIONTASK_H
#define DXREFLECTIONTASK_H

#include "GraphicsPass.h"

class Shader;
class ShaderBindingTableGenerator;

class IGraphicsBuffer;

enum E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION
{
	RootParamLocal_SRV_T6,
	RootParamLocal_CBV_T7,
	RootParamLocal_CBV_B8,
	NUM_LOCAL_PARAMS
};

class DXRReflectionTask : public GraphicsPass
{
public:
	DXRReflectionTask(unsigned int dispatchWidth, unsigned int dispatchHeight);
	~DXRReflectionTask();

	// Call this whenever new instances has been added/or removed from the rayTraced-scene
	void CreateShaderBindingTable(const std::vector<RenderComponent>& rayTracedRenderComponents);

	void Execute() override final;

private:
	// Shaders
	Shader* m_pRayGenShader = nullptr;
	Shader* m_pHitShader = nullptr;
	Shader* m_pMissShader = nullptr;

	// Local root signatures
	ID3D12RootSignature* m_pRayGenSignature;
	ID3D12RootSignature* m_pHitSignature;
	ID3D12RootSignature* m_pMissSignature;

	// Shader binding table
	ShaderBindingTableGenerator* m_pSbtGenerator = nullptr;
	IGraphicsBuffer* m_pShaderTableBuffer = nullptr;
	ID3D12StateObject* m_pStateObject = nullptr;
	ID3D12StateObjectProperties* m_pRTStateObjectProps = nullptr;

	unsigned int m_DispatchWidth = 0, m_DispatchHeight = 0;
};

#endif