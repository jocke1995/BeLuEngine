#ifndef DEFERREDLIGHTASK_H
#define DEFERREDLIGHTASK_H

#include "RenderTask.h"

class ShaderResourceView;

class mesh;

class DeferredLightRenderTask : public RenderTask
{
public:
	DeferredLightRenderTask(ID3D12Device5* device,
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds, 
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	~DeferredLightRenderTask();

	void SetFullScreenQuad(Mesh* mesh);

	void Execute() override final;

private:
	SlotInfo m_Info;
	Mesh* m_pFullScreenQuadMesh = nullptr;

};

#endif