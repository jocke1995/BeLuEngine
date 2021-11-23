#ifndef DXREFLECTIONTASK_H
#define DXREFLECTIONTASK_H

#include "DXRTask.h"

class Shader;
class ShaderBindingTableGenerator;
class Resource;

enum E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION
{
	RootParamLocal_SRV_T6,
	RootParamLocal_CBV_T7,
	RootParamLocal_CBV_B8,
	NUM_LOCAL_PARAMS
};

struct Resource_UAV_SRV;

class DXRReflectionTask : public DXRTask
{
public:
	DXRReflectionTask(ID3D12Device5* device,
		ID3D12RootSignature* globalRootSignature,
		Resource_UAV_SRV* resourceUavSrv,
		unsigned int width, unsigned int height,
		unsigned int FLAG_THREAD);
	~DXRReflectionTask();

	// Call this whenever new instances has been added/or removed from the rayTraced-scene
	void CreateShaderBindingTable(ID3D12Device5* device, const std::vector<RenderComponent>& rayTracedRenderComponents);

	void Execute() override final;

private:
	// Shaders
	Shader* m_pRayGenShader = nullptr;
	Shader* m_pHitShader = nullptr;
	Shader* m_pMissShader = nullptr;

	// Global Root signature
	ID3D12RootSignature* m_pGlobalRootSig;

	// Local root signatures
	ID3D12RootSignature* m_pRayGenSignature;
	ID3D12RootSignature* m_pHitSignature;
	ID3D12RootSignature* m_pMissSignature;

	// Shader binding table
	ShaderBindingTableGenerator* m_pSbtGenerator = nullptr;
	Resource* m_pSbtStorage = nullptr;
	ID3D12StateObject* m_pStateObject = nullptr;
	ID3D12StateObjectProperties* m_pRTStateObjectProps = nullptr;

	// Texture
	Resource_UAV_SRV* m_pResourceUavSrv;

	unsigned int m_DispatchWidth = 0, m_DispatchHeight = 0;
};

#endif