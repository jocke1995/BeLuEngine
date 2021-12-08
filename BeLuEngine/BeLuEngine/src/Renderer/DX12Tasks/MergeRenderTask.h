#ifndef MERGERENDERTASK_H
#define MERGERENDERTASK_H

#include "RenderTask.h"
class Mesh;

class IGraphicsTexture;

class MergeRenderTask : public RenderTask
{
public:
	MergeRenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		unsigned int FLAG_THREAD);
	virtual ~MergeRenderTask();
	
	void SetFullScreenQuad(Mesh* mesh);
	void CreateSlotInfo();

	void Execute() override final;
private:
	Mesh* m_pFullScreenQuadMesh = nullptr;

	SlotInfo m_Info;
	DescriptorHeapIndices m_dhIndices;
	unsigned int m_NumIndices;

};

#endif
