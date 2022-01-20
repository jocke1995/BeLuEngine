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


	void Execute() override final;

	void SetRenderComponents(const std::vector<RenderComponent>& renderComponents);
private:
	IRayTracingPipelineState* m_pRayTracingState = nullptr;

	// Shader binding table
	IShaderBindingTable* m_pShaderBindingTable = nullptr;
	void createShaderBindingTable();
	std::vector<RenderComponent> m_RenderComponents;

	unsigned int m_DispatchWidth = 0, m_DispatchHeight = 0;
};

#endif