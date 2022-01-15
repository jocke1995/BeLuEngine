#ifndef DXREFLECTIONTASK_H
#define DXREFLECTIONTASK_H

#include "GraphicsPass.h"

class IGraphicsBuffer;

class IRayTracingPipelineState;
class IShaderBindingTable;

enum E_LOCAL_ROOTSIGNATURE_DXR_REFLECTION
{
	RootParamLocal_SRV_T6,
	RootParamLocal_SRV_T7,
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
	IRayTracingPipelineState* m_pRayTracingState = nullptr;

	IShaderBindingTable* m_pShaderBindingTable = nullptr;

	unsigned int m_DispatchWidth = 0, m_DispatchHeight = 0;
};

#endif