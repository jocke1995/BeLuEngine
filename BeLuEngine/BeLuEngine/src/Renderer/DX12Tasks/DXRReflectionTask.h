#ifndef DXREFLECTIONTASK_H
#define DXREFLECTIONTASK_H

#include "DXRTask.h"

class Shader;
class ShaderBindingTableGenerator;

class IGraphicsBuffer;
class IGraphicsTexture;

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
	DXRReflectionTask(IGraphicsTexture* reflectionTexture, unsigned int width, unsigned int height, unsigned int FLAG_THREAD);
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

	// Texture
	IGraphicsTexture* m_pReflectionTexture;

	unsigned int m_DispatchWidth = 0, m_DispatchHeight = 0;
};

#endif