#ifndef DOWNSAMPLERENDERTASK_H
#define DOWNSAMPLERENDERTASK_H

#include "RenderTask.h"
class Mesh;

class IGraphicsTexture;

class DownSampleRenderTask : public RenderTask
{
public:
	DownSampleRenderTask(
		const std::wstring& VSName, const std::wstring& PSName,
		std::vector<D3D12_GRAPHICS_PIPELINE_STATE_DESC*>* gpsds,
		const std::wstring& psoName,
		IGraphicsTexture* sourceTexture,
		IGraphicsTexture* destinationTexture,
		unsigned int FLAG_THREAD);
	virtual ~DownSampleRenderTask();
	
	void SetFullScreenQuad(Mesh* mesh);
	void SetFullScreenQuadInSlotInfo();

	void Execute() override final;
private:
	IGraphicsTexture* m_pSourceTexture = nullptr;
	IGraphicsTexture* m_pDestinationTexture = nullptr;

	Mesh* m_pFullScreenQuadMesh = nullptr;

	SlotInfo m_Info;
	DescriptorHeapIndices m_dhIndices;

	unsigned int m_NumIndices;
};

#endif
