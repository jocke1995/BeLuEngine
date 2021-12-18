#ifndef MERGERENDERTASK_H
#define MERGERENDERTASK_H

#include "GraphicsPass.h"
class Mesh;

class IGraphicsTexture;

class MergeRenderTask : public GraphicsPass
{
public:
	MergeRenderTask(Mesh* fullscreenQuad);
	virtual ~MergeRenderTask();

	void Execute() override final;
private:
	Mesh* m_pFullScreenQuadMesh = nullptr;
};

#endif
