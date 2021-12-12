#ifndef DOWNSAMPLERENDERTASK_H
#define DOWNSAMPLERENDERTASK_H

#include "GraphicsPass.h"
class Mesh;

class IGraphicsTexture;

class DownSampleRenderTask : public GraphicsPass
{
public:
	DownSampleRenderTask(IGraphicsTexture* sourceTexture, IGraphicsTexture* destinationTexture, Mesh* fullscreenQuad);
	virtual ~DownSampleRenderTask();
	
	void Execute() override final;
private:
	IGraphicsTexture* m_pSourceTexture = nullptr;
	IGraphicsTexture* m_pDestinationTexture = nullptr;

	Mesh* m_pFullScreenQuadMesh = nullptr;
};

#endif
